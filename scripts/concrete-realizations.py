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
from random import shuffle
from sage.all import *
from sage.matroids.realization_space import *
from sage.matroids.realization_space import _is_poly_ring
matplotlib.use("Agg")


def open_sz(path):
    proc = subprocess.Popen(
        ["scripts/szcat.sh", path],
        stdout=subprocess.PIPE,
        text=True,
    )
    return proc.stdout


def has_solution(R, eqs, ineqs, q):
    F = GF(q)
    gens = list(R.gens())
    # order variables by how many constraints touch them (most-constrained first)
    appearance = Counter()
    for p in list(eqs) + list(ineqs):
        for v in p.variables():
            appearance[v] += 1
    order = sorted(range(len(gens)), key=lambda i: -appearance[gens[i]])
    variables = [gens[i] for i in order]
    n = len(variables)
    var_index = {v: i for i, v in enumerate(variables)}
    domains = [list(F) for _ in range(n)]

    def positions(poly):
        return set(var_index[v] for v in poly.variables())

    reduced_eqs, reduced_ineqs = [], []
    for f in eqs:
        pos = positions(f)
        if not pos:          # constant
            if F(f) != 0:
                return None  # 0 == nonzero constant; infeasible
            continue
        if len(pos) == 1:    # prune this variable's domain
            i = next(iter(pos))
            v = variables[i]
            domains[i] = [c for c in domains[i] if f(**{str(v): c}) == 0]
        else:
            reduced_eqs.append((f, pos))
    for g in ineqs:
        pos = positions(g)
        if not pos:
            if F(g) == 0:
                return None
            continue
        if len(pos) == 1:
            i = next(iter(pos))
            v = variables[i]
            domains[i] = [c for c in domains[i] if g(**{str(v): c}) != 0]
        else:
            reduced_ineqs.append((g, pos))
    if any(len(d) == 0 for d in domains):
        return None

    # bucket remaining constraints by the last variable (in chosen order)
    eqs_by_level = [[] for _ in range(n)]
    ineqs_by_level = [[] for _ in range(n)]
    for f, pos in reduced_eqs:
        eqs_by_level[max(pos)].append(f)
    for g, pos in reduced_ineqs:
        ineqs_by_level[max(pos)].append(g)

    assignment = [None] * n

    def backtrack(k):
        if k == n:
            return True
        for val in domains[k]:
            assignment[k] = val
            subs = {str(variables[i]): assignment[i] for i in range(k + 1)}
            if all(f(**subs) == 0 for f in eqs_by_level[k]) and \
               all(g(**subs) != 0 for g in ineqs_by_level[k]):
                if backtrack(k + 1):
                    return True
        assignment[k] = None
        return False

    if backtrack(0):
        return dict(zip(variables, assignment))
    return None


def element_to_int(a):
    """Convert a finite field element to its integer representation."""
    try:
        return a.integer_representation()
    except AttributeError:
        # Fallback for older Sage / givaro elements
        F = a.parent()
        p = F.characteristic()
        poly = a.polynomial()  # polynomial over GF(p)
        return int(sum(int(coeff) * (p ** i) for i, coeff in enumerate(poly)))


def matrix_to_json(A, q):
    """Serialize a matrix over `GF(q)` into JSON-safe data."""
    return {
        "q": q,
        "nrows": A.nrows(),
        "ncols": A.ncols(),
        "entries": [
            [element_to_int(A[i, j]) for j in range(A.ncols())]
            for i in range(A.nrows())
        ],
    }


def load_results(path):
    """
    Load a previously saved properties file:
    {matroid_colex_str: {"realization_space": ..., ...}}, where
    "realization_space" is the realization space of the matroid over `ZZ`, or
    `None`.
    """
    if not os.path.exists(path):
        return {}
    try:
        with open(path) as fh:
            return json.load(fh)
    except json.JSONDecodeError:
        print(
            f"Warning: {path} is not valid JSON, ignoring and starting fresh.")
        return {}


def save_results(results, path):
    def _numeric_key(k):
        try:
            return (0, int(k))
        except ValueError:
            return (1, k)

    ordered = {
        colex: results[colex]
        for colex in sorted(results)
    }

    tmp_path = f"{path}.tmp"
    with open(tmp_path, "w") as fh:
        json.dump(ordered, fh, indent=2)
    os.replace(tmp_path, path)


def _reduce_to_GF_q(f, q):
    """
    Reduce a ring element `f` (over `ZZ` or `ZZ[vars]`) to `GF(q)`.
    """
    if hasattr(f, "change_ring"):
        return f.change_ring(GF(q))
    return GF(q)(f)


def _process_one(colex, M, q_list, rs_cached):
    """
    Find the first GF(q) solution for a single matroid.

    `rs_cached` is either a JSON-safe dict for RS (the realization space of
    M over `ZZ`) or `None`.

    Returns "realization_space" and "concrete_rs".
    """
    if rs_cached is None:
        return {"colex": colex, "realization_space": None, "concrete_rs": None}

    RS = MatroidRealizationSpace.from_dict(rs_cached)
    concrete_rs_json = None
    sol = None

    for q in q_list:
        R = RS.ambient_ring
        if _is_poly_ring(R):
            eqs = [f for f in RS.defining_ideal.gens() if f != 0]
            ineqs = [g for g in RS.inequations]
            sol = has_solution(R, eqs, ineqs, q)
            if sol is None:
                continue
            A = RS.realization_matrix.subs(sol).change_ring(GF(q))
        else:
            if any([GF(q)(g) != 0 for g in RS.defining_ideal.gens()]) or \
               any([GF(q)(g) == 0 for g in RS.inequations]):
                continue
            A = RS.realization_matrix.change_ring(GF(q))

        assert A.base_ring() is GF(q), f'{colex}: matrix base ring not GF({q})'
        M_A = Matroid(A)
        assert M.is_isomorphic(M_A), f'{colex}: isomorphism check failed; \
            GF({q}), {RS.defining_ideal}, {RS.inequations}, {sol}'

        p, _ = is_prime_power(q, get_data=True)
        one_RS = MatroidRealizationSpace(
            RS.basis, GF(q).ideal([GF(q)(0)]), [], GF(q), A, p, q, GF(q)
        )
        one_RS.one_realization = True
        one_RS._is_realizable = True
        concrete_rs_json = one_RS.to_dict()
        break

    return {"colex": colex, "realization_space": RS.to_dict(),
            "concrete_rs": concrete_rs_json}


def process_batch(batch):
    """
    Worker: run _process_one for a batch of (colex, M, q_list, rs_cached)
    tuples in a single forked subprocess (more efficient).
    """
    return [_process_one(*task) for task in batch]


def save_q_histogram(results, pngpath, r, n):
    """Build and save a histogram of the q values found across all matroids."""
    q_values = [res["concrete_rs"]["q"] for res in results
                if res["concrete_rs"] is not None]

    fig, ax = plt.subplots(figsize=(8, 5))

    if q_values:
        distinct_qs = sorted(set(q_values))
        counts = Counter(q_values)
        print(counts)
        heights = [counts[q] for q in distinct_qs]
        xpos = range(len(distinct_qs))

        ax.bar(xpos, heights, color="#4C72B0", edgecolor="black")
        ax.set_xticks(xpos)
        ax.set_xticklabels([str(q) for q in distinct_qs])
        ax.set_xlabel("q")
        ax.set_ylabel("Number of matroids")
        ax.set_title(
            f"Distribution of minimum realizing field size q (r={r}, n={n})")

        n_realizable = sum(1 for res in results
                           if res["realization_space"] is not None and
                           res["realization_space"]["is_realizable"])
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

    prev_results = load_results(inpath_props)
    print(f"Loaded {len(prev_results)} cached results (incl. realization spaces) "
          f"from {inpath_props}")

    all_matroids = open_sz(inpath_matroids).read().splitlines()
    shuffle(all_matroids)
    total = len(all_matroids)

    results = {}
    tasks = []
    for colex in all_matroids:
        M = Matroid(rank=r, groundset=range(n), colex=colex)
        rs_cached = prev_results.get(colex, {}).get("realization_space")
        if rs_cached is None or not rs_cached["is_realizable"]:
            results[colex] = {"realization_space": None, "concrete_rs": None}
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
    for (call_args, call_kwargs), batch_result in worker(batches):
        for result in batch_result:
            done += 1
            colex = result.pop("colex")
            results[colex] = result
            found_q = result["concrete_rs"]["q"] if result["concrete_rs"] else None
            is_realizable = result["realization_space"]["is_realizable"] \
                if result["realization_space"] is not None else False
            print(f'[{done}/{len(tasks)}] {colex}: q={found_q}')

    save_results(results, outpath)
    print(f"Saved results for {len(results)} matroids to {outpath}")

    save_q_histogram(
        [results[colex] for colex in all_matroids], pngpath, r, n
    )
    print(f"Saved q histogram to {pngpath}")


if __name__ == "__main__":
    main()
