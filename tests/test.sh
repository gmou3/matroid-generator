#!/bin/bash

test_dir=$(dirname "$0")
if [ ! -f "$test_dir/../build/IC" ]; then
  echo "Error: IC executable not found"
  exit 1
fi

flag=true
N=8  # test all matroids up to N elements
for ((n = 0; n <= N; n++)); do
  for ((r = 0; r <= n; r++)); do
    expected=$(< "$test_dir/expected/n0${n}r0${r}")
    output=$("$test_dir/../build/IC" $n $r)
    if [ "$expected" != "$output" ]; then
        echo "Test failed for n=$n, r=$r"
        flag=false
    fi
  done
done

if $flag; then
  echo "All tests passed!"
else
  exit 1
fi
