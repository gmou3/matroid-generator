#!/usr/bin/env bash

DIR="$(dirname "$(readlink -f "$0")")/../build"

if [ "$#" -eq 0 ]; then
    echo "Usage: szcat [-i] <file>" >&2
    exit 1
fi

"$DIR/sz" "$@" -d -o /dev/stdout
