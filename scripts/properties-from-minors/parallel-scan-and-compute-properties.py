import argparse
import glob
import json
import multiprocessing as mp
import os
import subprocess
import time

mp.set_start_method("fork", force=True)

from datetime import timedelta
from math import comb
from sage.interfaces.singular import singular
from sage.matroids.constructor import Matroid
from sage.matroids.database_matroids import K33dual, K5dual


def fmt(r, n):
    return f"r{r:02d}n{n:02d}"


def build_properties(M):
    T = M.tutte_polynomial()
    return {
        'loopless': not M.loops(),
        'simple': M.is_simple(),
        'connected': M.is_connected(),
        'paving': M.is_paving(),
        'realizable': M.is_realizable(),
        'binary': M.is_binary(),
        'ternary': M.is_ternary(),
        'quaternary': M.is_quaternary(),
        'regular': M.is_regular(),
        'graphic': M.is_graphic(),
        'T20': int(T(2, 0)),
        'T02': int(T(0, 2)),
        'T11': int(T(1, 1)),
        'beta_invariant': int(M.beta_invariant()),
    }


def open_sz(path):
    proc = subprocess.Popen(
        ["scripts/szcat.sh", path],
        stdout=subprocess.PIPE,
        text=True,
    )
    return proc.stdout


parser = argparse.ArgumentParser()
parser.add_argument('--R', type=int, required=True)
parser.add_argument('--N', type=int, required=True)
parser.add_argument('--save-detailed-results', action='store_true')
parser.add_argument('-T', '--threads', type=int, default=1)
args = parser.parse_args()

R, N, save_detailed_results, threads = \
    args.R, args.N, args.save_detailed_results, args.threads

properties_contraction = {}
properties_deletion = {}


def _ensure_worker(worker):
    if worker is None or not worker[2].is_alive():
        q_in, q_out = mp.Queue(), mp.Queue()
        proc = mp.Process(target=_realizable_worker, args=(q_in, q_out))
        proc.start()
        return (q_in, q_out, proc)
    return worker


def _shutdown_worker(worker):
    if worker is not None and worker[2].is_alive():
        worker[0].put(None)
        worker[2].join()


def _realizable_worker(q_in, q_out):
    singular.quit()
    while True:
        item = q_in.get()
        if item is None:
            break
        M, c, basis = item
        RS = M.realization_space(characteristic=c, basis=basis)
        result = RS.is_realizable()
        q_out.put(result)


def is_realizable_with_timeout(M, worker, timeout=5):
    prev_basis = None
    for c in [2, 3, 5, 7, 11, None]:
        for basis in [prev_basis] + list(M.bases()):
            worker = _ensure_worker(worker)
            worker[0].put((M, c, basis))
            try:
                result = worker[1].get(timeout=timeout)
                prev_basis = basis
                if result or c is None:
                    return result, worker
                break
            except:
                try:
                    worker[2].kill()
                except:
                    pass
                worker = None
    raise TimeoutError


def process_part(args):
    file_rn, file_all, file_idx = args

    start = time.time()

    part_name = os.path.splitext(os.path.basename(file_rn))[0]
    worker = None

    lim = comb(N - 1, R)
    coloop = "0" * comb(N - 1, R)
    uniform = "*" * comb(N - 1, R - 1)

    K33d = K33dual()
    K5d = K5dual()

    cnt = {
        'all': 0,
        'loopless': 0,
        'coloopless': 0,
        'simple': 0,
        'connected': 0,
        'paving': 0,
        'realizable': 0,
        'binary': 0,
        'ternary': 0,
        'quaternary': 0,
        'regular': 0,
        'graphic': 0,
    }
    properties_by_matroid = {}

    with open_sz(file_rn) as rn_stream, \
            open_sz(file_all) as all_stream, \
            open(file_idx) as idx_stream:

        all_line_no = 0
        current_all_str = all_stream.readline()[:-1]
        current_canonical_idx = int(idx_stream.readline())
        contraction = properties_contraction[current_canonical_idx]

        for rn_line in rn_stream:
            rn_line = rn_line[:-1]
            prefix = rn_line[:lim]
            suffix = rn_line[lim:]

            while current_all_str < suffix:
                current_all_str = all_stream.readline()[:-1]
                current_canonical_idx = int(idx_stream.readline())
                contraction = properties_contraction[current_canonical_idx]
                all_line_no += 1

            assert current_all_str == suffix, \
                f"Suffix {suffix} not found in {fmt(R - 1, N - 1)}-all at line {all_line_no}"

            c_loopless = contraction['loopless']
            c_simple = contraction['simple']
            c_connected = contraction['connected']
            c_paving = contraction['paving']
            c_realizable = contraction['realizable']
            c_binary = contraction['binary']
            c_ternary = contraction['ternary']
            c_quaternary = contraction['quaternary']
            c_regular = contraction['regular']
            c_graphic = contraction['graphic']
            c_T20 = contraction['T20']
            c_T02 = contraction['T02']
            c_T11 = contraction['T11']
            c_beta_invariant = contraction['beta_invariant']

            is_realizable = False
            is_binary = False
            is_ternary = False
            is_quaternary = False
            is_regular = False
            is_graphic = False

            if prefix == coloop:
                is_loopless = c_loopless
                is_simple = c_simple
                is_connected = False
                is_paving = (suffix == uniform)
                T20 = 2 * c_T20
                T02 = 0
                T11 = c_T11
                beta_invariant = (N == 1)

                if is_loopless:
                    cnt['loopless'] += 1
                if is_simple:
                    cnt['simple'] += 1
                if is_paving:
                    cnt['paving'] += 1

                if c_realizable:
                    is_realizable = True
                    cnt['realizable'] += 1
                    if c_binary:
                        is_binary = True
                        cnt['binary'] += 1
                        if c_regular:
                            is_regular = True
                            cnt['regular'] += 1
                            if c_graphic:
                                is_graphic = True
                                cnt['graphic'] += 1
                    if c_ternary:
                        is_ternary = True
                        cnt['ternary'] += 1
                    if c_quaternary:
                        is_quaternary = True
                        cnt['quaternary'] += 1

            else:
                deletion = properties_deletion[prefix]
                d_loopless = deletion['loopless']
                d_simple = deletion['simple']
                d_connected = deletion['connected']
                d_paving = deletion['paving']
                d_realizable = deletion['realizable']
                d_binary = deletion['binary']
                d_ternary = deletion['ternary']
                d_quaternary = deletion['quaternary']
                d_graphic = deletion['graphic']
                d_T20 = deletion['T20']
                d_T02 = deletion['T02']
                d_T11 = deletion['T11']
                d_beta_invariant = deletion['beta_invariant']

                T20 = d_T20 + c_T20
                T02 = d_T02 + c_T02
                T11 = d_T11 + c_T11

                is_loopless = d_loopless
                is_simple = d_simple and c_loopless
                is_connected = d_connected or c_connected
                is_paving = d_paving and c_paving
                beta_invariant = d_beta_invariant + c_beta_invariant

                if d_loopless:
                    cnt['loopless'] += 1
                    assert T20 * \
                        T02 >= T11 ** 2, f"{rn_line}, {T20}, {T02}, {T11}"

                cnt['coloopless'] += 1
                if is_simple:
                    cnt['simple'] += 1
                if is_connected:
                    cnt['connected'] += 1
                if is_paving:
                    cnt['paving'] += 1

                if d_realizable and c_realizable:
                    M = Matroid(groundset=range(N), rank=R, revlex=rn_line)

                    try:
                        is_realizable, worker = is_realizable_with_timeout(M, worker)
                    except TimeoutError:
                        print(f"Part {part_name} aborted due to timeout on matroid {rn_line}")
                        return {}, {}

                    if is_realizable:
                        cnt['realizable'] += 1
                        if d_ternary and c_ternary and M.is_ternary():
                            is_ternary = True
                            cnt['ternary'] += 1
                        if d_quaternary and c_quaternary and M.is_quaternary():
                            is_quaternary = True
                            cnt['quaternary'] += 1
                        if d_binary and c_binary and M.is_binary():
                            is_binary = True
                            cnt['binary'] += 1
                            if is_ternary:
                                is_regular = True
                                cnt['regular'] += 1
                                if d_graphic and c_graphic and \
                                        not M.has_minor(K33d) and not M.has_minor(K5d):
                                    is_graphic = True
                                    cnt['graphic'] += 1

            if save_detailed_results:
                properties_by_matroid[rn_line] = {
                    'loopless':   is_loopless,
                    'coloopless': prefix != coloop,
                    'simple':     is_simple,
                    'connected':  is_connected,
                    'paving':     is_paving,
                    'realizable': is_realizable,
                    'binary':     is_binary,
                    'ternary':    is_ternary,
                    'quaternary': is_quaternary,
                    'regular':    is_regular,
                    'graphic':    is_graphic,
                    'T20': T20, 'T02': T02, 'T11': T11,
                    'beta_invariant': beta_invariant,
                }

            cnt['all'] += 1
            if cnt['all'] % 100 == 0:
                print(f'  {cnt["realizable"]} / {cnt["all"]}', end='\r')

        if save_detailed_results:
            with open(f"output/{part_name}-properties.json", 'w') as f:
                json.dump(dict(sorted(properties_by_matroid.items())), f)

        with open(f"output/{part_name}-properties-counts.json", 'w') as f:
            json.dump(cnt, f)

        _shutdown_worker(worker)

        elapsed = time.time() - start
        print(
            f"Part {part_name} done: {cnt['realizable']} / {cnt['all']}"
            f" ({timedelta(seconds=int(elapsed))})"
        )

        return cnt, properties_by_matroid


print("Reading canonical minors and computing properties...")
FILE_DELETION = f"output/{fmt(R, N - 1)}.sz"
JSON_DELETION = f"output/{fmt(R, N - 1)}-properties.json"
FILE_CONTRACTION = f"output/{fmt(R - 1, N - 1)}.sz"
JSON_CONTRACTION = f"output/{fmt(R - 1, N - 1)}-properties.json"

# Deletion properties
if os.path.exists(JSON_DELETION):
    print(f"  Reading deletion properties from {JSON_DELETION}...")
    with open(JSON_DELETION) as f:
        properties_deletion = json.load(f)
else:
    print(f"  Computing deletion properties with Sage from {FILE_DELETION}...")
    with open_sz(FILE_DELETION) as f:
        for line in f:
            line = line.strip()
            M = Matroid(rank=R, groundset=range(N - 1), revlex=line)
            properties_deletion[line] = build_properties(M)

# Contraction properties
if os.path.exists(JSON_CONTRACTION):
    print(f"  Reading contraction properties from {JSON_CONTRACTION}...")
    with open(JSON_CONTRACTION) as f:
        raw = json.load(f)
    properties_contraction = {i: props for i,
                              (_, props) in enumerate(raw.items())}
else:
    print(
        f"  Computing contraction properties with Sage from {FILE_CONTRACTION}...")
    with open_sz(FILE_CONTRACTION) as f:
        for i, line in enumerate(f):
            line = line.strip()
            M = Matroid(rank=R - 1, groundset=range(N - 1), revlex=line)
            properties_contraction[i] = build_properties(M)

FILE_RN_SUFFIX_SORTED = f"output/{fmt(R, N)}-suffix-sorted.sz"
FILE_CONTRACTION_ALL = f"output/{fmt(R - 1, N - 1)}-all.sz"
FILE_CONTRACTION_ALL_TO_IDX = f"output/{fmt(R - 1, N - 1)}-all-to-canonical_idx.txt"

part_files = sorted(glob.glob(f"output/{fmt(R, N)}-suffix-sorted-*.sz"))

if part_files and threads > 1:
    print(
        f"Performing main linear scan over {len(part_files)} parts with {threads} subprocesses...")

    # Find the last completed file's index, and reorder to start right after it
    def is_done(pf):
        part_name = os.path.splitext(os.path.basename(pf))[0]
        return os.path.exists(f"output/{part_name}-properties-counts.json")

    done_flags = [is_done(pf) for pf in part_files]
    last_done_idx = -1
    for i, d in enumerate(done_flags):
        if d:
            last_done_idx = i

    n = len(part_files)
    if last_done_idx >= 0:
        order = [part_files[(last_done_idx + 1 + i) % n] for i in range(n)]
    else:
        order = part_files[:]

    # Skip parts that already have completed output
    pending_part_files = []
    for pf in order:
        part_name = os.path.splitext(os.path.basename(pf))[0]
        counts_file = f"output/{part_name}-properties-counts.json"
        if not os.path.exists(counts_file):
            pending_part_files.append(pf)

    print(f'Pending parts: {len(pending_part_files)} / {len(part_files)}')

    worker_args = [
        (pf, FILE_CONTRACTION_ALL, FILE_CONTRACTION_ALL_TO_IDX)
        for i, pf in enumerate(pending_part_files)
    ]

    procs = []
    for wa in worker_args:
        while len(procs) >= threads:
            procs = [pr for pr in procs if pr.is_alive()]
            if len(procs) >= threads:
                time.sleep(1)

        p = mp.Process(target=process_part, args=(wa,))
        p.start()
        procs.append(p)

    for p in procs:
        p.join()

    # Aggregate from per-part files written on disk (all parts, including skipped ones)
    cnt = {}
    properties_by_matroid = {}
    for pf in part_files:
        part_name = os.path.splitext(os.path.basename(pf))[0]
        counts_file = f"output/{part_name}-properties-counts.json"
        if not os.path.exists(counts_file):
            print(
                f"  WARNING: missing {counts_file}, skipping (part may be stuck/failed)")
            continue
        with open(counts_file) as f:
            part_cnt = json.load(f)
        for k, v in part_cnt.items():
            cnt[k] = cnt.get(k, 0) + v

        if save_detailed_results:
            props_file = f"output/{part_name}-properties.json"
            if os.path.exists(props_file):
                with open(props_file) as f:
                    properties_by_matroid.update(json.load(f))
            else:
                print(
                    f"  WARNING: {part_name} has counts but no detailed properties (was it run with --save-detailed-results?)")

else:
    print("Performing main linear scan...")
    cnt, properties_by_matroid = process_part((
        FILE_RN_SUFFIX_SORTED,
        FILE_CONTRACTION_ALL,
        FILE_CONTRACTION_ALL_TO_IDX,
    ))

for property, cnt_property in cnt.items():
    print(f'  {property}: {cnt_property}')

if save_detailed_results:
    JSON_FILE = f"output/{fmt(R, N)}-properties.json"
    with open(JSON_FILE, 'w') as f:
        json.dump(dict(sorted(properties_by_matroid.items())), f)
        print(f"Detailed results saved in {JSON_FILE}")

JSON_FILE = f"output/{fmt(R, N)}-properties-counts.json"
with open(JSON_FILE, 'w') as f:
    json.dump(cnt, f)
    print(f"Counts saved in {JSON_FILE}")
