#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <input_dir> <merged_output> <index_output>" >&2
    exit 1
fi

INPUT_DIR="$1"
MERGED_OUT="$2"
INDEX_OUT="$3"

files=()
while IFS= read -r line; do
  files+=("$line")
done < <(find "$INPUT_DIR" -maxdepth 1 -name '*.sz.zst' | sort)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "Error: no .sz.zst files found in '$INPUT_DIR'" >&2
    exit 1
fi

export LC_ALL=C
ulimit -n 200000

echo "Found ${#files[@]} files:" >&2
for i in "${!files[@]}"; do
    echo "  [$i] ${files[$i]}" >&2
done

# Create a temp dir for FIFOs, cleaned up on exit
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

fifos=()
for i in "${!files[@]}"; do
    fifo="$TMPDIR/input_$i"
    mkfifo "$fifo"
    fifos+=("$fifo")
    # Feed each FIFO in the background
    scripts/szzstdcat.sh "${files[$i]}" \
        | awk -v idx="$i" '{print idx, $0}' \
        > "$fifo" &
done

sort -m -k2 -T "$INPUT_DIR" -S 2G --compress-program="scripts/zstd-level.sh" \
    --batch-size=64 "${fifos[@]}" \
| awk '{
    print $2 | "build/sz | scripts/zstd-level.sh > '"$MERGED_OUT"'"
    print $1 | "scripts/zstd-level.sh > '"$INDEX_OUT"'"
}'

wait
echo "Done. Written: $MERGED_OUT, $INDEX_OUT" >&2
