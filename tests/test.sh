#!/usr/bin/env bash

test_dir=$(dirname "$0")
pushd "$test_dir" >/dev/null

if [ ! -f "../build/IC" ]; then
  echo "Error: IC executable not found"
  exit 1
fi

flag=true
N=8
echo "Testing for all matroids with up to $N elements..."
for ((n = 0; n <= N; n++)); do
  for ((r = 0; r <= n; r++)); do
    # Test serial version
    expected=$(< "expected/n0${n}r0${r}")
    output=$("../build/IC" $n $r)
    if [ "$expected" != "$output" ]; then
        echo "Test failed for n=$n, r=$r (serial)"
        flag=false
    fi

    # Test parallel version with file output
    "../build/IC" $n $r 4 --file
    output=$(< "output/n0${n}r0${r}")
    if [ "$expected" != "$output" ]; then
        echo "Test failed for n=$n, r=$r (parallel with file output)"
        flag=false
    fi
  done
done

rm -rf output
popd >/dev/null

if $flag; then
  echo "All tests passed!"
else
  exit 1
fi
