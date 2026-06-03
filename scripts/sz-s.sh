#!/usr/bin/env bash
# Wrapper for streaming version of sz to be used in `sort --compress-program=`

exec sz -s "$@"
