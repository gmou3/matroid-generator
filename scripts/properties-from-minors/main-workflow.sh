#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage:"
    echo "./scripts/properties-from-minors/main-workflow.sh <n> <r> [threads]"
    exit 1
}

[[ $# -lt 2 ]] && usage

export LC_ALL=C

N=$1
R=$2
THREADS=${3:-1}

N1=$((N - 1))
R1=$((R - 1))

# Zero-pad to 2 digits
fmt() { printf "%02d" "$1"; }

NN=$(fmt "$N");   RR=$(fmt "$R")
NN1=$(fmt "$N1"); RR1=$(fmt "$R1")

# Compute required canonical matroids
run_ic() {
    local n=$1 r=$2
    local nn; nn=$(fmt "$n"); local rr; rr=$(fmt "$r")
    local out="output/n${nn}r${rr}.sz"
    if [[ -f "$out" ]]; then
        echo "- Skipping IC ($n, $r): $out already exists"
    else
        echo "- Running IC ($n, $r)"
        "build/IC" "$n" "$r" "$THREADS" --compressed-file
    fi
}

NR_MATROIDS="output/n${NN}r${RR}.sz"
NR_MATROIDS_SUFFIX="output/n${NN}r${RR}-suffix-sorted.sz"
N1R1_MATROIDS="output/n${NN1}r${RR1}.sz"
N1R1_MATROIDS_ALL_DIR="output/n${NN1}r${RR1}-all"
N1R1_MATROIDS_ALL="output/n${NN1}r${RR1}-all.sz"
N1R1_CANONICAL_IDX="output/n${NN1}r${RR1}-all-to-canonical_idx.txt.xz"

choose() {
    local n=$1 r=$2
    awk -v n="$n" -v r="$r" 'BEGIN {
        if (r > n) { print 0; exit }
        if (r == 0 || r == n) { print 1; exit }
        if (r > n-r) r = n-r
        c = 1
        for (i = 0; i < r; i++) c = c * (n-i) / (i+1)
        printf "%d\n", c
    }'
}

TOTAL=$(choose "$N" "$R")
SUFFIX_LEN=$(choose "$((N-1))" "$((R-1))")
PREFIX_LEN=$(choose "$((N-1))" "$R")
SUFFIX_START=$((TOTAL - SUFFIX_LEN + 1))

# Sort (n, r) by (n - 1, r - 1) suffix
if [[ -f "$NR_MATROIDS_SUFFIX" ]]; then
    echo "- Skipping sort ($N, $R) by suffix: $NR_MATROIDS_SUFFIX already exists"
else
    run_ic "$N" "$R"
    echo "- Sorting ($N, $R) canonical matroids by suffix"
    "scripts/szcat.sh" "$NR_MATROIDS" \
        | sort -k1.${SUFFIX_START},1.${TOTAL} -k1.1,1.${PREFIX_LEN} \
            -T "output" -S 16G --parallel=${THREADS} \
            --compress-program="scripts/sz-s.sh" \
        | "build/sz" /dev/stdin -o "$NR_MATROIDS_SUFFIX"
fi

# Compute all colex permutations of (n - 1, r - 1)
if [[ -d "$N1R1_MATROIDS_ALL_DIR" || -f "$N1R1_MATROIDS_ALL" ]]; then
    echo "- Skipping colex permutations: output already exists"
else
    run_ic "$N1" "$R1"
    echo "- Computing colex permutations for ($N1, $R1)"
    python3 scripts/properties-from-minors/colex-permutations.py "$N1" "$R1" "$N1R1_MATROIDS" "$N1R1_MATROIDS_ALL_DIR" "-T${THREADS}"
fi

# Merge sorted files of colex permutations
if [[ -f "$N1R1_CANONICAL_IDX" ]]; then
    echo "- Skipping merge: $N1R1_CANONICAL_IDX already exists"
else
    echo "- Merging sorted colex files"
    scripts/properties-from-minors/merge-sorted.sh "$N1R1_MATROIDS_ALL_DIR/" "$N1R1_MATROIDS_ALL" "$N1R1_CANONICAL_IDX"
fi

# Main parallel linear scan
run_ic "$N1" "$R"
echo "- Running property computation for ($N, $R)"
OPTIONS="--N $N --R $R"
if [ "$N" -lt 13 ] && ! { [ "$N" -eq 10 ] && { [ "$R" -eq 4 ] || [ "$R" -eq 5 ] || [ "$R" -eq 6 ]; }; }; then
    OPTIONS="$OPTIONS --save-results"
fi
sage -python scripts/properties-from-minors/parallel-scan-and-compute-properties.py $OPTIONS
