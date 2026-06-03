#!/usr/bin/env bash
# Wrapper for streaming version of sz to be used in `sort --compress-program=`

DIR="$(dirname "$(readlink -f "$0")")/../build"
exec "$DIR/sz" -s "$@"
