#!/usr/bin/env bash

DIR="$(dirname "$(readlink -f "$0")")"

if ! command -v xzcat &>/dev/null; then
    echo "xzcat not found" >&2
    exit 1
fi

if [ "$#" -eq 0 ]; then
    echo "Usage: szxzcat [-i] <file>" >&2
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

xzcat "$FILE" | "$DIR/szcat.sh" "${FLAGS[@]}" /dev/stdin
