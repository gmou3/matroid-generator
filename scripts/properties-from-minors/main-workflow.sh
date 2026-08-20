#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage:"
    echo "./scripts/properties-from-minors/main-workflow.sh <r> <n> [threads] [--realizable] [--char <c>]"
    exit 1
}

[[ $# -lt 2 ]] && usage

export LC_ALL=C

R=$1
N=$2
shift 2

THREADS=1
REALIZABLE=0
CHAR=""

if [[ $# -gt 0 && "$1" =~ ^[0-9]+$ ]]; then
    THREADS=$1
    shift
fi
if [[ $# -gt 0 ]]; then
    [[ "$1" == "--realizable" ]] || usage
    REALIZABLE=1
    shift
fi
if [[ $# -gt 0 ]]; then
    [[ "$1" == "--char" ]] || usage
    [[ $# -ge 2 ]] || usage
    shift
    CHAR=$1
    shift
fi
[[ $# -eq 0 ]] || usage

R1=$((R - 1))
N1=$((N - 1))

# Zero-pad to 2 digits
fmt() { printf "%02d" "$1"; }

RR=$(fmt "$R");   NN=$(fmt "$N")
RR1=$(fmt "$R1"); NN1=$(fmt "$N1")

# Compute required canonical matroids
run_ic() {
    local r=$1 n=$2
    local rr; rr=$(fmt "$r"); local nn; nn=$(fmt "$n")
    local out="output/r${rr}n${nn}.sz"
    if [[ -f "$out" ]]; then
        echo "- Skipping IC ($r, $n): $out already exists"
    else
        echo "- Running IC ($r, $n)"
        "build/IC" "$r" "$n" "$THREADS" --compressed-file
    fi
}

RN_MATROIDS="output/r${RR}n${NN}.sz"
RN_MATROIDS_SUFFIX="output/r${RR}n${NN}-suffix-sorted.sz"
RN_MATROIDS_SUFFIX_PATTERN="output/r${RR}n${NN}-suffix-sorted*.sz"
R1N1_MATROIDS="output/r${RR1}n${NN1}.sz"
R1N1_MATROIDS_ALL_DIR="output/r${RR1}n${NN1}-all"
R1N1_MATROIDS_ALL="output/r${RR1}n${NN1}-all.sz"
R1N1_CANONICAL_IDX="output/r${RR1}n${NN1}-all-to-canonical_idx.txt"

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

# Sort (r, n)-matroids by C(n - 1, r - 1) suffix
if compgen -G "$RN_MATROIDS_SUFFIX_PATTERN" > /dev/null 2>&1; then
    echo "- Skipping sort ($R, $N) by suffix: $RN_MATROIDS_SUFFIX_PATTERN already exists"
else
    run_ic "$R" "$N"
    echo "- Sorting ($R, $N) canonical matroids by suffix"
    "scripts/szcat.sh" "$RN_MATROIDS" \
        | sort -k1.${SUFFIX_START},1.${TOTAL} -k1.1,1.${PREFIX_LEN} \
            -T "output" -S 16G --parallel=${THREADS} \
            --compress-program="scripts/sz-s.sh" \
        | "build/sz" /dev/stdin -o "$RN_MATROIDS_SUFFIX"
fi

run_ic "$R1" "$N1"
# Compute all colex permutations of (n - 1, r - 1)
if [[ -d "$R1N1_MATROIDS_ALL_DIR" || -f "$R1N1_MATROIDS_ALL" ]]; then
    echo "- Skipping colex permutations: output already exists"
else
    echo "- Computing colex permutations for ($R1, $N1)"
    python3 scripts/properties-from-minors/helpers/colex-permutations.py "$R1" "$N1" "$R1N1_MATROIDS" "$R1N1_MATROIDS_ALL_DIR" "-T${THREADS}"
fi

# Merge sorted files of colex permutations
if [[ -f "$R1N1_CANONICAL_IDX" ]]; then
    echo "- Skipping merge: $R1N1_CANONICAL_IDX already exists"
else
    echo "- Merging sorted colex files"
    scripts/properties-from-minors/helpers/merge-sort.sh "$R1N1_MATROIDS_ALL_DIR/" "$R1N1_MATROIDS_ALL" "$R1N1_CANONICAL_IDX"
fi

# Main parallel linear scan
run_ic "$R" "$N1"
echo "- Running property computation for ($R, $N)"
PY_ARGS=(--R "$R" --N "$N" --threads "$THREADS")
if [ "$N" -lt 13 ] && ! { [ "$N" -eq 10 ] && { [ "$R" -eq 4 ] || [ "$R" -eq 5 ] || [ "$R" -eq 6 ]; }; }; then
    PY_ARGS+=(--save-detailed-results)
fi
if [[ "$REALIZABLE" -eq 1 ]]; then
    PY_ARGS+=(--realizable)
fi
if [[ -n "$CHAR" ]]; then
    PY_ARGS+=(--char "$CHAR")
fi
sage -python scripts/properties-from-minors/parallel-scan-and-compute-properties.py "${PY_ARGS[@]}"
