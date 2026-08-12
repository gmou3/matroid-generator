#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <input.sz> <lines_per_part>" >&2
    exit 1
fi

INPUT="$1"
LINES_PER_PART="$2"

PREFIX="${INPUT%%.*}"

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

"scripts/szcat.sh" "$INPUT" \
| awk -v lpp="$LINES_PER_PART" -v tmpdir="$TMPDIR" -v prefix="$PREFIX" '
BEGIN { part=0; fifo=""; }
{
    if ((NR-1) % lpp == 0) {
        if (fifo != "") close(fifo)
        fifo = sprintf("%s/part_%06d", tmpdir, part)
        out  = sprintf("%s-part%06d.sz", prefix, part)
        system("mkfifo " fifo)
        system("build/sz " fifo " -o " out " &")
        part++
    }
    print > fifo
}
END { if (fifo != "") close(fifo) }
'

wait
echo "Done." >&2
