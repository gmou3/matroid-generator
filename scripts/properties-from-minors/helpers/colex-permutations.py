from itertools import combinations, permutations
import argparse
import os
import sys
import subprocess
from concurrent.futures import ProcessPoolExecutor, as_completed
from math import comb

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

def build_tables(n, r):
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

def all_permutations(input_str, n, all_subsets, subset_to_idx, total):
    active, active_char = parse_input(input_str, total)
    seen = set()
    for pi in permutations(range(n)):
        image = apply_permutation(active, pi, all_subsets, subset_to_idx)
        if image not in seen:
            seen.add(image)
            yield pi, to_string(image, active_char, total)

def process_seed(args):
    seed_idx, input_str, out_dir, n, r = args
    all_subsets, subset_to_idx, total = build_tables(n, r)
    orbit = sorted(s for _, s in all_permutations(input_str, n, all_subsets, subset_to_idx, total))
    sz_file = os.path.join(out_dir, f"n{n:02d}r{r:02d}-{seed_idx:06d}.sz")

    subprocess.run(
        ["build/sz", "/dev/stdin", "-o", sz_file],
        input=("\n".join(orbit) + "\n").encode(),
        check=True,
    )
    # subprocess.run(["xz", "-9e", sz_file], check=True)

    return seed_idx, len(orbit)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int, help="Number of elements")
    parser.add_argument("r", type=int, help="Rank")
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
            tasks.append((seed_idx, input_str, args.out_dir, args.n, args.r))
            seed_idx += 1

    done = 0
    with ProcessPoolExecutor(max_workers=args.threads) as executor:
        futures = {executor.submit(process_seed, t): t[0] for t in tasks}
        for future in as_completed(futures):
            try:
                seed_idx, cnt = future.result()
                done += 1
                print(f"{done}/{len(tasks)}", file=sys.stderr, end='\r')
            except Exception as e:
                print(f"\n[seed {futures[future]}] failed: {e}", file=sys.stderr)

    print(f"\nWritten {done} files to {args.out_dir}", file=sys.stderr)
