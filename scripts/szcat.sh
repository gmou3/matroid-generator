#!/usr/bin/env bash

SZ="$(dirname "$(readlink -f "$0")")/../build/sz"

"$SZ" "$@" -d -o /dev/stdout
