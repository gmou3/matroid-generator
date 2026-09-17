#!/usr/bin/env bash
# Wrapper for sz+xz to be used in `sort --compress-program=`
set -eo pipefail

DIR="$(dirname "$(readlink -f "$0")")/../build"

if [ "$1" = "-d" ]; then
    xz -d | "$DIR/sz" -d
else
    "$DIR/sz" -s | xz -9e
fi
