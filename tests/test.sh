#!/usr/bin/env bash

test_dir=$(dirname "$0")
pushd "$test_dir" >/dev/null

executable="../build/IC"
if [ ! -f $executable ]; then
    echo "Error: IC executable not found"
    exit 1
fi

extend_executable="../build/IC-extend"
if [ ! -f $extend_executable ]; then
    echo "Error: IC-extend executable not found"
    exit 1
fi

flag=true
extension_output=$($extend_executable 2 4 "******")
expected_extensions=$'**********\n0000******'
if [ "$extension_output" != "$expected_extensions" ]; then
    echo "Test failed: IC-extend (2, 4, ******)"
    flag=false
fi

N=8
echo "Testing for all matroids with up to $N elements..."
for ((r = 0; r <= N; r++)); do
    for ((n = r; n <= N; n++)); do
        # Test serial version
        expected=$(< "expected/r0${r}n0${n}")
        output=$($executable $r $n)
        if [ "$expected" != "$output" ]; then
            echo "Test failed: ($r, $n)"
            flag=false
        fi

        # Test parallel version with file output
        $executable $r $n 2 --file
        output=$(< "output/r0${r}n0${n}")
        if [ "$expected" != "$output" ]; then
            echo "Test failed: ($r, $n, 2, --file)"
            flag=false
        fi

        # Test parallel version with compressed file output
        $executable $r $n 4 --compressed-file
        output=$(../scripts/szcat.sh "output/r0${r}n0${n}.sz")
        if [ "$expected" != "$output" ]; then
            echo "Test failed: ($r, $n, 4, --compressed-file)"
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
