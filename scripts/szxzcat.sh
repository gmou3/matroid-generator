#!/usr/bin/env bash

DIR="$(dirname "$(readlink -f "$0")")"

if ! command -v xzcat &>/dev/null; then
    echo "xzcat not found" >&2
    exit 1
fi

xzcat "$1" | "$DIR/szcat.sh" /dev/stdin
