from itertools import combinations, permutations
import argparse
import os
import sys
import subprocess
from math import comb, factorial

# One BLAS/OpenMP thread per worker: the work here is already parallel across
# seeds, so letting each worker start its own thread pool only oversubscribes.
os.environ.setdefault("OMP_NUM_THREADS", "1")
os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")
os.environ.setdefault("MKL_NUM_THREADS", "1")

import multiprocessing as mp
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np

def open_sz(path):
    proc = subprocess.Popen(
        ["scripts/szcat.sh", path],
        stdout=subprocess.PIPE,
        text=True,
    )
    return proc.stdout

def colex_rank(subset):
    s = sorted(subset)
    return sum(comb(s[i], i + 1) for i in range(len(s)))

def build_tables(r, n):
    total = comb(n, r)
    all_subsets = sorted(combinations(range(n), r), key=colex_rank)
    assert len(all_subsets) == total
    subset_to_idx = {frozenset(s): i for i, s in enumerate(all_subsets)}
    return all_subsets, subset_to_idx, total

def parse_input(s, total):
    assert len(s) == total, f"Expected {total} chars, got {len(s)}"
    assert all(c in '0*' for c in s), "Characters must be '0' or '*'"
    zeros = frozenset(i for i, c in enumerate(s) if c == '0')
    stars = frozenset(i for i, c in enumerate(s) if c == '*')
    if len(zeros) <= len(stars):
        return zeros, '0'
    else:
        return stars, '*'

# The table has n! * C(n, r) entries, so it grows out of memory fast: (4, 9) is
# 45MB but (5, 10) would already be ~900MB. Past this many elements, fall back
# to relabeling each seed's subsets one permutation at a time.
MAX_TABLE_N = 9

def build_perm_table(r, n):
    """Return T with T[i, p] = colex index of the image of the i-th r-subset
    under the p-th permutation of the ground set, in itertools order.

    The action of the symmetric group on the r-subsets is the same for every
    matroid, so it is worth computing once instead of per seed: each seed then
    costs a handful of table lookups per permutation rather than rebuilding
    every image from scratch.
    """
    all_subsets, _, total = build_tables(r, n)
    nperm = factorial(n)

    subsets = np.array(all_subsets, dtype=np.int16)                 # (total, r)
    perms = np.array(list(permutations(range(n))), dtype=np.int16)   # (nperm, n)

    # comb_tab[e, j] = C(e, j), for the colex rank of a sorted subset.
    comb_tab = np.array([[comb(e, j) for j in range(r + 1)]
                         for e in range(n)], dtype=np.int64)

    idx_dtype = np.uint8 if total <= np.iinfo(np.uint8).max else np.uint16
    out = np.empty((total, nperm), dtype=idx_dtype)

    # Chunk over permutations so that the intermediate (chunk, total, r) array
    # stays bounded regardless of n.
    chunk = max(1, (1 << 24) // max(1, total * r))
    for start in range(0, nperm, chunk):
        stop = min(start + chunk, nperm)
        images = perms[start:stop][:, subsets]     # (chunk, total, r)
        images.sort(axis=2)                        # colex rank wants it sorted
        rank = np.zeros(images.shape[:2], dtype=np.int64)
        for j in range(r):
            rank += comb_tab[images[:, :, j], j + 1]
        out[:, start:stop] = rank.T.astype(idx_dtype)

    return out

# Built once in the parent and inherited by the workers through fork, so the
# table costs one copy for the whole pool rather than one per worker.
TABLE = None
ROWS = None

def orbit_from_table(input_str, r, n):
    """Sorted, deduplicated orbit of `input_str` under relabelings of the
    ground set, as a numpy array of fixed-width byte strings."""
    total = comb(n, r)
    active, active_char = parse_input(input_str, total)
    active_byte = ord(active_char)
    inactive_byte = ord('*' if active_char == '0' else '0')

    nperm = TABLE.shape[1]
    grid = np.full((nperm, total), inactive_byte, dtype=np.uint8)
    if active:
        A = np.fromiter(sorted(active), dtype=np.intp, count=len(active))
        # ROWS broadcasts against TABLE[A], which is (len(active), nperm), so
        # this scatters every active subset's image for every permutation.
        grid[ROWS, TABLE[A]] = active_byte

    # 'S<total>' compares with memcmp, matching bytes ordering and LC_ALL=C.
    orbit = np.unique(grid.view(f'S{total}').ravel())

    m = orbit.shape[0]
    buf = np.empty((m, total + 1), dtype=np.uint8)
    buf[:, :total] = orbit.view(np.uint8).reshape(m, total)
    buf[:, total] = ord('\n')
    return m, buf.tobytes()

def apply_permutation(active_indices, pi, all_subsets, subset_to_idx):
    result = set()
    for idx in active_indices:
        subset = all_subsets[idx]
        permuted = frozenset(pi[x] for x in subset)
        result.add(subset_to_idx[permuted])
    return frozenset(result)

def to_string(active_indices, active_char, total):
    inactive_char = '*' if active_char == '0' else '0'
    return ''.join(active_char if i in active_indices else inactive_char for i in range(total))

def orbit_by_relabeling(input_str, r, n):
    """Same orbit as `orbit_from_table`, built one permutation at a time so it
    needs no precomputed table."""
    all_subsets, subset_to_idx, total = build_tables(r, n)
    active, active_char = parse_input(input_str, total)

    seen = set()
    for pi in permutations(range(n)):
        image = apply_permutation(active, pi, all_subsets, subset_to_idx)
        if image not in seen:
            seen.add(image)
    orbit = sorted(to_string(image, active_char, total) for image in seen)
    return len(orbit), ("\n".join(orbit) + "\n").encode()

def process_seed(args):
    seed_idx, input_str, out_dir, r, n = args
    if TABLE is not None:
        count, payload = orbit_from_table(input_str, r, n)
    else:
        count, payload = orbit_by_relabeling(input_str, r, n)
    sz_file = os.path.join(out_dir, f"r{r:02d}n{n:02d}-{seed_idx:06d}.sz")

    subprocess.run(
        ["build/sz", "/dev/stdin", "-o", sz_file],
        input=payload,
        check=True,
    )

    return seed_idx, count

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("r", type=int, help="Rank")
    parser.add_argument("n", type=int, help="Number of elements")
    parser.add_argument("input", help="Input file")
    parser.add_argument("out_dir", nargs="?", default=".", help="Output directory")
    parser.add_argument("-T", "--threads", type=int, default=1)
    args = parser.parse_args()

    assert 1 <= args.r <= args.n, f"Must have 1 <= r <= n, got n={args.n}, r={args.r}"
    total = comb(args.n, args.r)

    os.makedirs(args.out_dir, exist_ok=True)

    tasks = []
    with open_sz(args.input) as f:
        seed_idx = 0
        for line in f:
            input_str = line.strip()
            if not input_str:
                continue
            tasks.append((seed_idx, input_str, args.out_dir, args.r, args.n))
            seed_idx += 1

    if args.n <= MAX_TABLE_N:
        TABLE = build_perm_table(args.r, args.n)
        ROWS = np.arange(TABLE.shape[1], dtype=np.intp)

    done = 0
    with ProcessPoolExecutor(max_workers=args.threads,
                             mp_context=mp.get_context("fork")) as executor:
        futures = {executor.submit(process_seed, t): t[0] for t in tasks}
        for future in as_completed(futures):
            try:
                seed_idx, cnt = future.result()
                done += 1
                print(f"  {done}/{len(tasks)}", file=sys.stderr, end='\r')
            except Exception as e:
                print(f"\n  [seed {futures[future]}] failed: {e}", file=sys.stderr)

    print(f"\nWritten {done} files to {args.out_dir}", file=sys.stderr)
