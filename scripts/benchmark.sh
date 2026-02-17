#!/usr/bin/env bash
set -euo pipefail
pushd "$(dirname "$0")/.." >/dev/null

ref=$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse HEAD)
trap 'git checkout "$ref" --quiet 2>/dev/null || true; popd >/dev/null' EXIT

if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "Error: You have uncommitted changes. Stash or commit them first."
  exit 1
fi

executable="./build/IC"
n="${1:-8}"
r="${2:-4}"
num_threads="${3:-}"

for commit in $(git log --format="%H" 672ac65^..HEAD); do
  git checkout "$commit" --quiet
  git log --oneline -n1
  make --quiet
  hyperfine "$executable $n $r $num_threads"
done
