#!/usr/bin/env bash

DIR="$(dirname "$(readlink -f "$0")")/../build"

if [ "$#" -ne 1 ]; then
    echo "Usage: szcat <file>" >&2
    exit 1
fi

"$DIR/sz" "$1" -d -o /dev/stdout
