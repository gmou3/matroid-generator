#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <r> <n> <input_dir>" >&2
    exit 1
fi

R="$1"
N="$2"
INPUT_DIR="$3"

export LC_ALL=C
ulimit -n 200000

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

echo "String length:  C($N,$R)     = $TOTAL" >&2
echo "Suffix length:  C($((N-1)),$((R-1))) = $SUFFIX_LEN" >&2
echo "Prefix length:  C($((N-1)),$R) = $PREFIX_LEN" >&2

mapfile -t files < <(find "$INPUT_DIR" -maxdepth 1 -name '*.xz' | sort)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "Error: no .xz files found in '$INPUT_DIR'" >&2
    exit 1
fi

echo "Found ${#files[@]} files" >&2

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

fifos=()
for i in "${!files[@]}"; do
    fifo="$TMPDIR/input_$i"
    mkfifo "$fifo"
    fifos+=("$fifo")
    scripts/szcat.sh "${files[$i]}" > "$fifo" &
done

SUFFIX_START=$((TOTAL - SUFFIX_LEN + 1))

MERGED_OUT="output/$(printf 'r%02dn%02d-suffix-sorted.sz' "$R" "$N")"

sort -m \
    -k1.${SUFFIX_START},1.${TOTAL} \
    -k1.1,1.${PREFIX_LEN} \
    "${fifos[@]}" \
| build/sz /dev/stdin -o "$MERGED_OUT"

wait
echo "Done. Written: $MERGED_OUT" >&2
