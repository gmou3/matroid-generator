import argparse
import subprocess
import sys
import json
import os
import glob
import multiprocessing as mp
from math import comb
from sage.matroids.constructor import Matroid
from sage.matroids.database_matroids import K33dual, K5dual, Fano, FanoDual

def fmt(n, r):
    return f"n{n:02d}r{r:02d}"

def open_sz(path):
    proc = subprocess.Popen(
        ["scripts/szcat.sh", path],
        stdout=subprocess.PIPE,
        text=True,
    )
    return proc.stdout

parser = argparse.ArgumentParser()
parser.add_argument('--N', type=int, required=True)
parser.add_argument('--R', type=int, required=True)
parser.add_argument('--save-detailed-results', action='store_true')
parser.add_argument('-T', '--threads', type=int, default=1)
args = parser.parse_args()

N, R, save_detailed_results, threads = \
 args.N, args.R, args.save_detailed_results, args.threads

properties_contraction = {}
properties_deletion = {}

def process_part(args):
    file_nr, file_all, file_idx = args

    lim = comb(N - 1, R)
    coloop = "0" * comb(N - 1, R)
    uniform = "*" * comb(N - 1, R - 1)

    K33d = K33dual()
    K5d = K5dual()
    F7 = Fano()
    F7d = FanoDual()

    cnt = {
        'all': 0,
        'loopless': 0,
        'coloopless': 0,
        'simple': 0,
        'connected': 0,
        'paving': 0,
        'binary': 0,
        'ternary': 0,
        'quaternary': 0,
        'regular': 0,
        'graphic': 0,
    }
    properties_by_matroid = {}

    with open_sz(file_nr) as nr_stream, \
         open_sz(file_all) as all_stream, \
         open(file_idx) as idx_stream:

        all_line_no = 0
        current_all_str = all_stream.readline()[:-1]
        current_canonical_idx = int(idx_stream.readline())
        contraction = properties_contraction[current_canonical_idx]

        for nr_line in nr_stream:
            nr_line = nr_line[:-1]
            prefix = nr_line[:lim]
            suffix = nr_line[lim:]

            while current_all_str < suffix:
                current_all_str = all_stream.readline()[:-1]
                current_canonical_idx = int(idx_stream.readline())
                contraction = properties_contraction[current_canonical_idx]
                all_line_no += 1

            assert current_all_str == suffix, \
                f"Suffix {suffix} not found in {fmt(N-1,R-1)}-all at line {all_line_no}"

            c_loopless = contraction['loopless']
            c_simple = contraction['simple']
            c_connected = contraction['connected']
            c_paving = contraction['paving']
            c_binary = contraction['binary']
            c_ternary = contraction['ternary']
            c_quaternary = contraction['quaternary']
            c_regular = contraction['regular']
            c_graphic = contraction['graphic']
            c_T20 = contraction['T20']
            c_T02 = contraction['T02']
            c_T11 = contraction['T11']
            is_binary = False
            is_ternary = False
            is_quaternary = False
            is_regular = False
            is_graphic = False

            if prefix == coloop:
                is_loopless = c_loopless
                is_simple = c_simple
                is_connected = False
                is_paving = (suffix == uniform)  # unique paving with coloop
                T20 = 2 * c_T20
                T02 = 0
                T11 = c_T11

                if is_loopless:
                    cnt['loopless'] += 1
                if is_simple:
                    cnt['simple'] += 1
                if is_paving:
                    cnt['paving'] += 1

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
                d_binary = deletion['binary']
                d_ternary = deletion['ternary']
                d_quaternary = deletion['quaternary']
                d_regular = deletion['regular']
                d_graphic = deletion['graphic']
                d_T20 = deletion['T20']
                d_T02 = deletion['T02']
                d_T11 = deletion['T11']

                T20 = d_T20 + c_T20
                T02 = d_T02 + c_T02
                T11 = d_T11 + c_T11

                is_loopless = d_loopless
                is_simple = d_simple and c_loopless
                is_connected = d_connected or c_connected
                is_paving = d_paving and c_paving

                if d_loopless:
                    cnt['loopless'] += 1
                    # Merino-Welsh conjecture
                    assert T20 * T02 >= T11 ** 2, f"{nr_line}, {T20}, {T02}, {T11}"

                cnt['coloopless'] += 1
                if is_simple:
                    cnt['simple'] += 1
                if is_connected:
                    cnt['connected'] += 1
                if is_paving:
                    cnt['paving'] += 1

                if (d_binary and c_binary) or (d_ternary and c_ternary) or \
                   (d_quaternary and c_quaternary):
                    M = Matroid(groundset=range(N), rank=R, revlex=nr_line)
                    if d_ternary and c_ternary and M.is_ternary():
                        is_ternary = True
                        cnt['ternary'] += 1
                    if d_quaternary and c_quaternary and M.is_quaternary():
                        is_quaternary = True
                        cnt['quaternary'] += 1
                    if d_binary and c_binary and M.is_binary():
                        is_binary = True
                        cnt['binary'] += 1
                        if d_regular and c_regular and \
                           is_ternary and is_quaternary \
                           and not M.has_minor(F7) and not M.has_minor(F7d):
                            is_regular = True
                            cnt['regular'] += 1
                            if d_graphic and c_graphic \
                                    and not M.has_minor(K33d) and not M.has_minor(K5d):
                                is_graphic = True
                                cnt['graphic'] += 1

            if save_detailed_results:
                properties_by_matroid[nr_line] = {
                    'loopless':   is_loopless,
                    'coloopless': prefix != coloop,
                    'simple':     is_simple,
                    'connected':  is_connected,
                    'paving':     is_paving,
                    'binary':     is_binary,
                    'ternary':    is_ternary,
                    'quaternary': is_quaternary,
                    'regular':    is_regular,
                    'graphic':    is_graphic,
                    'T20': T20,
                    'T02': T02,
                    'T11': T11,
                }

            cnt['all'] += 1
            if not cnt['all'] % 1_000_000:
                print(f'  {cnt["all"] // 1_000_000} M', end='\r')

    return cnt, properties_by_matroid

print("Reading canonical minors and computing properties...")
FILE_CONTRACTION = f"output/{fmt(N-1, R-1)}.sz"
FILE_DELETION = f"output/{fmt(N-1, R)}.sz"
JSON_CONTRACTION = f"output/{fmt(N-1, R-1)}-properties.json"
JSON_DELETION = f"output/{fmt(N-1, R)}-properties.json"

# Contraction properties
if os.path.exists(JSON_CONTRACTION):
    print(f"  Reading contraction properties from {JSON_CONTRACTION}...")
    with open(JSON_CONTRACTION) as f:
        raw = json.load(f)
    properties_contraction = {i: props for i, (_, props) in enumerate(raw.items())}
else:
    print(f"  Computing contraction properties with Sage from {FILE_CONTRACTION}...")
    with open_sz(FILE_CONTRACTION) as f:
        for i, line in enumerate(f):
            line = line.strip()
            M = Matroid(groundset=range(N-1), rank=R-1, revlex=line)
            T = M.tutte_polynomial()
            properties_contraction[i] = {
                'loopless': not M.loops(),
                'simple': M.is_simple(),
                'connected': M.is_connected(),
                'paving': M.is_paving(),
                'binary': M.is_binary(),
                'ternary': M.is_ternary(),
                'quaternary': M.is_quaternary(),
                'regular': M.is_regular(),
                'graphic': M.is_graphic(),
                'T20': int(T(2, 0)),
                'T02': int(T(0, 2)),
                'T11': int(T(1, 1)),
            }

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
            M = Matroid(groundset=range(N-1), rank=R, revlex=line)
            T = M.tutte_polynomial()
            properties_deletion[line] = {
                'loopless': not M.loops(),
                'simple': M.is_simple(),
                'connected': M.is_connected(),
                'paving': M.is_paving(),
                'binary': M.is_binary(),
                'ternary': M.is_ternary(),
                'quaternary': M.is_quaternary(),
                'regular': M.is_regular(),
                'graphic': M.is_graphic(),
                'T20': int(T(2, 0)),
                'T02': int(T(0, 2)),
                'T11': int(T(1, 1)),
            }

FILE_N_R_SUFFIX_SORTED = f"output/{fmt(N, R)}-suffix-sorted.sz"
FILE_CONTRACTION_ALL = f"output/{fmt(N-1, R-1)}-all.sz"
FILE_CONTRACTION_ALL_TO_IDX = f"output/{fmt(N-1, R-1)}-all-to-canonical_idx.txt"

part_files = sorted(glob.glob(f"output/{fmt(N, R)}-suffix-sorted-*.sz"))

if part_files and threads > 1:
    print(f"Performing main linear scan over {len(part_files)} parts with {threads} threads...")

    worker_args = [
        (pf,
         FILE_CONTRACTION_ALL,
         FILE_CONTRACTION_ALL_TO_IDX)
        for pf in part_files
    ]

    with mp.Pool(threads) as pool:
        results = pool.map(process_part, worker_args)

    cnt = {k: sum(r[0][k] for r in results) for k in results[0][0]}
    if save_detailed_results:
        properties_by_matroid = {k: v for r in results for k, v in r[1].items()}

else:
    print("Performing main linear scan...")
    cnt, properties_by_matroid = process_part((
        FILE_N_R_SUFFIX_SORTED,
        FILE_CONTRACTION_ALL,
        FILE_CONTRACTION_ALL_TO_IDX,
    ))

for property, cnt_property in cnt.items():
    print(f'  {property}: {cnt_property}')

JSON_FILE = f"output/{fmt(N, R)}-properties.json"
with open(JSON_FILE, 'w') as f:
    if save_detailed_results:
        json.dump(dict(sorted(properties_by_matroid.items())), f)
        print(f"Detailed results saved in {JSON_FILE}")
    else:
        json.dump(cnt, f)
        print(f"Counts saved in {JSON_FILE}")
