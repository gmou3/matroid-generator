#!/usr/bin/env bash

test_dir=$(dirname "$0")
pushd "$test_dir" >/dev/null

executable="../build/IC"
if [ ! -f $executable ]; then
    echo "Error: IC executable not found"
    exit 1
fi

echo "Testing output for specific seed matroids..."
for seed_idx in 0 2500 5000 7500 10000 22500 25000 37500 60000 100000 115000 \
                122500 140000 160000 180000 190000 190214; do
    echo "Processing seed index $seed_idx..."
    time $executable $seed_idx
done

sha256sum -c "sha256sums"
exit_code=$?

rm -rf output
popd >/dev/null

if [ $exit_code -eq 0 ]; then
    echo "All tests passed!"
else
    exit $exit_code
fi
