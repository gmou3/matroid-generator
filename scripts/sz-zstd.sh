#!/usr/bin/env bash
# Wrapper for sz+zstd to be used in `sort --compress-program=`
set -eo pipefail

SZ="$(dirname "$(readlink -f "$0")")/../build/sz"

if [ "$1" = "-d" ]; then
    zstd -d | "$SZ" -d
else
    "$SZ" | scripts/zstd-level.sh
fi
