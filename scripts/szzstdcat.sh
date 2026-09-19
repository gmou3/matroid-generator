#!/usr/bin/env bash

DIR="$(dirname "$(readlink -f "$0")")"

if ! command -v zstdcat &>/dev/null; then
    echo "zstdcat not found" >&2
    exit 1
fi

if [ "$#" -eq 0 ]; then
    echo "Usage: szzstdcat [-i] <file>" >&2
    exit 1
fi

FLAGS=()
ARGS=()
for arg in "$@"; do
    case $arg in
        -i) FLAGS+=("-i") ;;
        *)  ARGS+=("$arg") ;;
    esac
done
FILE="${ARGS[0]}"

zstdcat "$FILE" | "$DIR/szcat.sh" "${FLAGS[@]}" /dev/stdin
