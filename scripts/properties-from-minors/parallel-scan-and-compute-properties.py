import argparse
import subprocess
import sys
import json
import os
from math import comb
from sage.matroids.constructor import Matroid

def fmt(n, r):
    return f"n{n:02d}r{r:02d}"

def open_xz(path):
    proc = subprocess.Popen(
        ["xzcat", path],
        stdout=subprocess.PIPE,
        text=True,
    )
    return proc.stdout

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
parser.add_argument('--save-results', action='store_true')
args = parser.parse_args()

N, R, save_results = args.N, args.R, args.save_results

print("Reading canonical minors and computing properties...")
FILE_CONTRACTION = f"output/{fmt(N-1, R-1)}.sz"
FILE_DELETION = f"output/{fmt(N-1, R)}.sz"
JSON_CONTRACTION = f"output/{fmt(N-1, R-1)}-properties.json"
JSON_DELETION = f"output/{fmt(N-1, R)}-properties.json"

properties_contraction = {}
properties_deletion = {}

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
            M = Matroid(groundset=range(N - 1), rank=R-1, revlex=line)
            T = M.tutte_polynomial()
            properties_contraction[i] = {
                'loopless': not M.loops(),
                'simple': M.is_simple(),
                'connected': M.is_connected(),
                'paving': M.is_paving(),
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
            M = Matroid(groundset=range(N - 1), rank=R, revlex=line)
            T = M.tutte_polynomial()
            properties_deletion[line] = {
                'loopless': not M.loops(),
                'simple': M.is_simple(),
                'connected': M.is_connected(),
                'paving': M.is_paving(),
                'T20': int(T(2, 0)),
                'T02': int(T(0, 2)),
                'T11': int(T(1, 1)),
            }

print("Performing main parallel linear scan...")
FILE_N_R_SUFFIX_SORTED = f"output/{fmt(N, R)}-suffix-sorted.sz"
FILE_CONTRACTION_ALL = f"output/{fmt(N-1, R-1)}-all.sz"
FILE_CONTRACTION_ALL_TO_IDX = f"output/{fmt(N-1, R-1)}-all-to-canonical_idx.txt.xz"

lim = comb(N - 1, R)
coloop = "0" * comb(N - 1, R)
loop = "0" * comb(N - 1, R - 1)
uniform = "*" * comb(N - 1, R - 1)
cnt = {
    'all': 0,
    'loopless': 0,
    'coloopless': 0,
    'simple': 0,
    'connected': 0,
    'paving': 0,
}

properties_by_matroid = {}

with open_sz(FILE_N_R_SUFFIX_SORTED) as nr_stream, \
     open_sz(FILE_CONTRACTION_ALL) as all_stream, \
     open_xz(FILE_CONTRACTION_ALL_TO_IDX) as idx_stream:

    all_line_no = 0
    current_all_str = all_stream.readline().rstrip('\n')
    current_canonical_idx = int(idx_stream.readline().strip())
    contraction = properties_contraction[current_canonical_idx]

    for nr_line in nr_stream:
        nr_line = nr_line.rstrip('\n')
        prefix = nr_line[:lim]
        suffix = nr_line[lim:]

        current_matroid = {
            'loopless': False,
            'coloopless': False,
            'simple': False,
            'connected': False,
            'paving': False,
            'T20': 0,
            'T02': 0,
            'T11': 0,
        }

        while current_all_str < suffix:
            current_all_str = all_stream.readline().rstrip('\n')
            current_canonical_idx = int(idx_stream.readline().strip())
            contraction = properties_contraction[current_canonical_idx]
            all_line_no += 1

        assert current_all_str == suffix, \
            f"Suffix {suffix} not found in {fmt(N-1,R-1)}-all at line {all_line_no}"

        if prefix == coloop:
            if contraction['loopless']:
                current_matroid['loopless'] = True
            if contraction['simple']:
                current_matroid['simple'] = True
            if suffix == uniform:
                # the unique paving matroid when n is a coloop (U_{N-1,R-1} + U_{1, 1})
                current_matroid['paving'] = True
            current_matroid['T20'] = 2 * contraction['T20']
            current_matroid['T02'] = 0 * contraction['T02']
            current_matroid['T11'] = 1 * contraction['T11']
        else:
            deletion = properties_deletion[prefix]
            T20 = current_matroid['T20'] = deletion['T20'] + contraction['T20']
            T02 = current_matroid['T02'] = deletion['T02'] + contraction['T02']
            T11 = current_matroid['T11'] = deletion['T11'] + contraction['T11']
            if deletion['loopless']:
                current_matroid['loopless'] = True
                # Merino-Welsh conjecture
                assert T20 * T02 >= T11 ** 2, \
                    f"{nr_line}, {T20}, {T02}, {T11}"
            current_matroid['coloopless'] = True
            if deletion['simple'] and contraction['loopless']:
                current_matroid['simple'] = True
            if deletion['connected'] or contraction['connected']:
                current_matroid['connected'] = True
            if deletion['paving'] and contraction['paving']:
                current_matroid['paving'] = True

        for property, value in current_matroid.items():
            if isinstance(value, bool) and value:
                cnt[property] += 1

        if save_results:
            properties_by_matroid[nr_line] = current_matroid

        cnt['all'] += 1
        if not cnt['all'] % 1000000:
            print(f'  {cnt['all'] // 1000000} M', end='\r')

if cnt['all'] >= 1000000:
    print()
for property, cnt_property in cnt.items():
    print(f'  {property}: {cnt_property}')

if save_results:
    JSON_FILE = f"output/{fmt(N, R)}-properties.json"
    with open(JSON_FILE, 'w') as f:
        json.dump(dict(sorted(properties_by_matroid.items())), f)
    print(f"Detailed results saved in {JSON_FILE}")
