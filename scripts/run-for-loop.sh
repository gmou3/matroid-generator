#!/usr/bin/env bash

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo "Usage: $0 <left_lim> <right_lim> [step]"
    exit 1
fi

pushd "$(dirname "$0")/.." >/dev/null
trap 'popd >/dev/null' EXIT

left_lim=$1
right_lim=$2
step=${3:-1}

szcat="./scripts/szcat.sh"
szxzcat="./scripts/szxzcat.sh"

sha256sums_file="./sha256sums"

export LC_ALL=C  # for proper behaviour of `sort` and `comm`

for seed_idx in $(seq "$left_lim" "$step" "$right_lim")
do
    echo "Processing seed index $seed_idx..."
    sz_file="output/n10r05-seedmatroid$(printf "%06d" $seed_idx).sz"
    if [ ! -f "$sz_file" ]; then
        time ./build/IC $seed_idx
        sha256sum "$sz_file" >> "$sha256sums_file"
    fi
    if [ ! -f "${sz_file}.xz" ]; then
        xz -9e -k "$sz_file"
        sha256sum "${sz_file}.xz" >> "$sha256sums_file"
    fi

    # Sanity checks
    "$szcat" "$sz_file" | sort -c
    str_len=$("$szcat" -i "$sz_file" | awk '{print $5}')
    if [ "$str_len" -ne "210" ]; then
        echo "$seed_idx: line length is not C(10, 5) = 252"
    fi
    cnt_szcat_i=$("$szcat" -i "$sz_file" | awk '{print $1}')
    cnt_szcat_wc=$("$szcat" "$sz_file" | wc -l)
    cnt_szxzcat_i=$("$szxzcat" -i "${sz_file}.xz" | awk '{print $1}')
    if [ "$cnt_szcat_i" -ne "$cnt_szcat_wc" ]; then
        echo "$seed_idx: counts of 'szcat -i' and 'szcat | wc -l' differ"
    fi
    if [ "$cnt_szcat_i" -ne "$cnt_szxzcat_i" ]; then
        echo "$seed_idx: counts of 'szcat -i' and 'szxzcat -i' differ"
    fi
    comm -3 <("$szcat" "$sz_file") <("$szxzcat" "${sz_file}.xz") | head -10
    xz -C sha256 --test "${sz_file}.xz"
done

sort -u -k2 "$sha256sums_file" -o "$sha256sums_file"  # sort and deduplicate
sha256sum -c "$sha256sums_file" | grep -v ': OK$'
