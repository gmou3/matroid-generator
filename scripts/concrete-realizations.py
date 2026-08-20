#!/usr/bin/env sage
"""
Find a `GF(q)` realization for all realizable (r, n)-matroids.

The `q` value found is the minimum one for which the matroid is realizable.

Usage:
    sage -python scripts/concrete-realizations.py r n [ncpus]
"""
import argparse
import json
import matplotlib
import matplotlib.pyplot as plt
import os
import subprocess
from collections import Counter
from pathlib import Path
from random import shuffle
from sage.all import *
from sage.matroids.realization_space import MatroidRealizationSpace
matplotlib.use("Agg")


def open_sz(path):
    proc = subprocess.Popen(
        ["scripts/szcat.sh", path],
        stdout=subprocess.PIPE,
        text=True,
    )
    return proc.stdout


def _process_one_matroid(colex, M, q_list, rs_cached):
    """
    Find the first `GF(q)` realization of matroid `M`, given `rs_cached`
    which is the JSON-stored precomputed realization space over `ZZ`.

    Returns the concrete realization space.
    """
    RS = MatroidRealizationSpace.from_dict(rs_cached)
    concrete_realization_space_json = None
    sol = None

    for q in q_list:
        RS_q = RS.concrete_realization(q)
        if RS_q is None:
            continue

        A = RS_q.realization_matrix
        M_A = Matroid(A)
        assert A.base_ring() is GF(q), f'{colex}: matrix base ring not GF({q})'
        assert M.equals(M_A), f'{colex}: matroid equality check failed; \
            GF({q})\n{A}'

        concrete_realization_space_json = RS_q.to_dict()
        break

    return {
        "colex": colex,
        "concrete_realization_space": concrete_realization_space_json
    }


def process_batch(batch):
    """
    Worker: run _process_one_matroid for a batch of (colex, M, q_list,
    rs_cached) tuples in a single forked subprocess (more efficient).
    """
    return [_process_one_matroid(*task) for task in batch]


def save_q_histogram(results, prev_results, pngpath, r, n):
    """
    Build and save a histogram of the `q` values found across all matroids.
    """
    q_values = [r["concrete_realization_space"]["q"] for r in results.values()
                if r["concrete_realization_space"] is not None]

    fig, ax = plt.subplots(figsize=(8, 5))

    if q_values:
        distinct_qs = sorted(set(q_values))
        counts = Counter(q_values)

        print(dict(sorted(counts.items())))
        jsonpath = Path(pngpath).with_suffix(".json")
        with open(jsonpath, "w") as f:
            json.dump(dict(sorted(counts.items())), f, indent=2)
        heights = [counts[q] for q in distinct_qs]
        xpos = range(len(distinct_qs))

        ax.bar(xpos, heights, color="#4C72B0", edgecolor="black")
        ax.set_xticks(xpos)
        ax.set_xticklabels([str(q) for q in distinct_qs])
        ax.set_xlabel("q")
        ax.set_ylabel("Number of matroids")
        ax.set_title(
            f"Distribution of minimum realizing field size q (r={r}, n={n})")

        n_realizable = sum(1 for r in prev_results.values()
                           if r["realization_space"] is not None and
                           r["realization_space"]["is_realizable"])
        n_found = len(q_values)
        n_not_found = n_realizable - n_found
        subtitle = f"{n_found}/{n_realizable} realizable matroids found their min q"
        if n_not_found:
            subtitle += f" ({n_not_found} realizable but no q found)"
        ax.text(
            0.5, 1.08, subtitle,
            transform=ax.transAxes, ha="center", va="bottom",
            fontsize=9, color="gray",
        )
    else:
        ax.text(
            0.5, 0.5, "No realizations found",
            transform=ax.transAxes, ha="center", va="center",
        )
        ax.set_title(
            f"Distribution of minimum realizing field size q (r={r}, n={n})")

    fig.tight_layout()
    fig.savefig(pngpath, dpi=150)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Find GF(q) realizations for all (r, n)-matroids."
    )
    parser.add_argument("r", type=int, help="rank of the matroids")
    parser.add_argument("n", type=int, help="number of elements")
    parser.add_argument("ncpus", type=int, nargs='?', default=1)
    args = parser.parse_args()

    r, n = args.r, args.n
    os.makedirs("output", exist_ok=True)
    inpath_matroids = f'output/r{r:02d}n{n:02d}.sz'
    inpath_props = f'output/r{r:02d}n{n:02d}-properties.json'
    outpath = f'output/r{r:02d}n{n:02d}-concrete-realizations.json'
    pngpath = f'output/r{r:02d}n{n:02d}-q-histogram.png'
    q_list = [q for q in range(2, 64 + 1) if is_prime_power(q)]

    prev_results = {}
    if os.path.exists(inpath_props):
        with open(inpath_props, "r") as f:
            prev_results = json.load(f)
    print(f"Loaded {len(prev_results)} cached results (incl. realization "
          f" spaces) from {inpath_props}")

    all_matroids = open_sz(inpath_matroids).read().splitlines()
    shuffle(all_matroids)
    total = len(all_matroids)

    results_unordered = {}
    tasks = []
    for colex in all_matroids:
        M = Matroid(rank=r, groundset=range(n), colex=colex)
        rs_cached = prev_results.get(colex, {}).get("realization_space")
        if rs_cached is None or not rs_cached["is_realizable"]:
            results_unordered[colex] = {"concrete_realization_space": None}
        else:
            tasks.append((colex, M, q_list, rs_cached))

    batch_size = 1000
    batches = [tasks[k:k + batch_size]
               for k in range(0, len(tasks), batch_size)]

    print(f"Processing {len(tasks)} / {total} matroids in {len(batches)} batches "
          f"of up to {batch_size} on {args.ncpus} worker processes "
          f"({total - len(tasks)} skipped: no cached realization space)...")

    worker = parallel(ncpus=args.ncpus)(process_batch)

    done = 0
    for _, batch_result in worker(batches):
        for result in batch_result:
            done += 1
            colex = result.pop("colex")
            results_unordered[colex] = result
            found_q = result["concrete_realization_space"]["q"] \
                      if result["concrete_realization_space"] else None
            print(f'[{done}/{len(tasks)}] {colex}: q={found_q}')

    results = {
        colex: results_unordered[colex]
        for colex in sorted(results_unordered)
    }

    tmp_path = f"{outpath}.tmp"
    with open(tmp_path, "w") as f:
        json.dump(results, f, indent=2)
    os.replace(tmp_path, outpath)
    print(f"Saved results for {len(results)} matroids to {outpath}")

    save_q_histogram(results, prev_results, pngpath, r, n)
    print(f"Saved q histogram to {pngpath}")


if __name__ == "__main__":
    main()
