from collections import Counter
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path
from sage.all import *
import argparse
import subprocess
"""
Find the excluded minors over `GF(q)` up to certain number of elements.

Usage:
    sage -python scripts/excluded-minors.py <q> <n_lim> [-T<threads>]
"""


def canonical_extensions(r, n, colex):
    executable = Path(__file__).resolve().parents[1] / 'build' / 'IC-extend'
    result = subprocess.run(
        [str(executable), str(r), str(n), colex],
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.splitlines()


def process_task(task):
    r, n, colex, q = task
    extensions = canonical_extensions(r, n, colex)
    candidates = [(r, extension) for extension in extensions[:-1]]

    realizable = []
    nonrealizable = []
    for child_r, child_colex in candidates:
        M = Matroid(rank=child_r, groundset=range(n + 1),
                    colex=child_colex)
        p, _ = is_prime_power(q, get_data=True)
        RS = M.realization_space(characteristic=p, simplify=False,
                                 compute_matrix=False)
        if RS.q_solution(q) is not None:
            realizable.append((child_r, child_colex))
        else:
            nonrealizable.append((child_r, child_colex))

    realizable.append((r + 1, extensions[-1]))  # coloop extension
    return realizable, nonrealizable


parser = argparse.ArgumentParser()
parser.add_argument('q', type=int, help='finite field size')
parser.add_argument('n_lim', type=int, help='maximum number of elements')
parser.add_argument('-T', '--threads', type=int, default=1,
                    help='number of worker processes (default: 1)')
args = parser.parse_args()

q = args.q
n_lim = args.n_lim
exc_minors = []
frontier = {1: {(1, '*')}}

if __name__ == '__main__':
    with ProcessPoolExecutor(max_workers=args.threads) as executor:
        for n in range(1, n_lim):
            tasks = [(r, n, colex, q) for r, colex in frontier.get(n, set())]
            next_frontier = set()
            results = executor.map(process_task, tasks)

            for realizable, nonrealizable in results:
                for child_r, child_colex in realizable + nonrealizable:
                    child_key = (child_r, child_colex)
                    if child_key in realizable:
                        next_frontier.add(child_key)
                    else:
                        child_n = n + 1
                        M = Matroid(rank=child_r, groundset=range(child_n),
                                    colex=child_colex)
                        if not M.is_simple() or not M.is_connected() or \
                           any(M.has_minor(N) for N in exc_minors
                               if N.rank() <= child_r
                               and len(N.groundset()) < child_n):
                            continue
                        exc_minors.append(M)
                        print(child_r, child_n, child_colex)

            frontier[n + 1] = next_frontier

    print(f"Total excluded minors for q={q} (n<={n_lim}): {len(exc_minors)}")
