#!/usr/bin/env bash

test_dir=$(dirname "$0")
pushd "$test_dir" >/dev/null

executable="../build/IC"
if [ ! -f $executable ]; then
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
        output=$($executable $n $r)
        if [ "$expected" != "$output" ]; then
            echo "Test failed: ($n, $r)"
            flag=false
        fi

        # Test parallel version with file output
        $executable $n $r 2 --file
        output=$(< "output/n0${n}r0${r}")
        if [ "$expected" != "$output" ]; then
            echo "Test failed: ($n, $r, 2, --file)"
            flag=false
        fi

        # Test parallel version with compressed file output
        $executable $n $r 4 --compressed-file
        output=$(xzcat "output/n0${n}r0${r}.xz")
        if [ "$expected" != "$output" ]; then
            echo "Test failed: ($n, $r, 4, --compressed-file)"
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
