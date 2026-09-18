#!/usr/bin/env bash
# Wrapper for sz+xz to be used in `sort --compress-program=`
set -eo pipefail

SZ="$(dirname "$(readlink -f "$0")")/../build/sz"

if [ "$1" = "-d" ]; then
    xz -d | "$SZ" -d
else
    "$SZ" | xz -9e
fi
