from collections import Counter
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from sage.all import *
import argparse
import subprocess
"""
Find the excluded minors over `GF(q)` up to certain number of elements.

Usage:
    sage -python scripts/excluded-minors.py <q> [<q> ...] <n_lim> [-T<threads>]
"""
parser = argparse.ArgumentParser()
parser.add_argument('q', type=int, nargs='+', help='finite field sizes')
parser.add_argument('n_lim', type=int, help='maximum number of elements')
parser.add_argument('-T', '--threads', type=int, default=1,
                    help='number of worker processes (default: 1)')
args = parser.parse_args()

qs = args.q
n_lim = args.n_lim


def canonical_extensions(r, n, colex):
    executable = Path(__file__).resolve().parents[1] / 'build' / 'IC-extend'
    proc = subprocess.Popen(
        [str(executable), str(r), str(n), colex],
        stdout=subprocess.PIPE,
        text=True,
    )
    try:
        for line in proc.stdout:
            yield line.rstrip('\n')
    finally:
        proc.stdout.close()
        proc.wait()
        if proc.returncode != 0:
            raise subprocess.CalledProcessError(proc.returncode, proc.args)


def process_task(task):
    task_id, ntasks, r, n, colex, exc_minors, qs, char = task
    extensions = canonical_extensions(r, n, colex)

    realizable = []
    new_exc_minors = []

    i = 0
    current = next(extensions)
    for nxt in extensions:
        i += 1
        np1 = n + 1
        M = Matroid(rank=r, groundset=range(np1), colex=current)
        if M.is_simple() and \
           all(len(F) <= max(qs) or len(F) == np1 for F in M.flats(2)):
            basis = next(iter(M.bases()))
            RS = M.realization_space(basis=basis, characteristic=char,
                                     simplify=False, compute_matrix=False)
            if all(RS.concrete_realization(q) is not None for q in qs):
                if np1 < n_lim:
                    realizable.append((r, current))
            elif 2 * r <= np1:
                if (M.is_cosimple() and
                    all(len(F) <= max(qs) or len(F) == np1
                        for F in M.dual().flats(2)) and
                    M.is_3connected() and
                    not any(M.has_minor(N)
                            for N in exc_minors if N.rank() <= r)):
                    new_exc_minors.append(M)
                    if 2 * r != np1:
                        new_exc_minors.append(M.dual())
                        print(f"{r} {np1} {current} (+D)")
                    else:
                        print(f"{r} {np1} {current}")
        if i % 1000 == 0:
            print(f'  ({n}: {task_id} / {ntasks}) {i}',
                  end='                \r')
        current = nxt

    if 2 * (r + 1) <= n_lim:
        realizable.append((r + 1, current))  # coloop extension
    return realizable, new_exc_minors


if __name__ == '__main__':
    exc_minors = []
    exc_by_n = Counter()
    frontier = [(1, '*')]

    char = None
    if len(qs) == 1:
        char, _ = is_prime_power(qs[0], get_data=True)

    with ProcessPoolExecutor(max_workers=args.threads) as executor:
        for n in range(1, n_lim):
            tasks = [(i, len(frontier), r, n, colex, exc_minors, qs, char)
                     for i, (r, colex) in enumerate(frontier)]

            results = list(executor.map(process_task, tasks))

            frontier.clear()
            for realizable, new_exc_minors in results:
                frontier.extend(realizable)
                exc_minors.extend(new_exc_minors)
                exc_by_n[n + 1] += len(new_exc_minors)

            frontier = sorted(frontier)

    print(dict(sorted(exc_by_n.items())))
    print(
        f"Total excluded minors for q={qs} (n ≤ {n_lim}): {exc_by_n.total()}")
