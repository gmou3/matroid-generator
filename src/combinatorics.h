#pragma once

#include <algorithm>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

constexpr size_t N = 10;     // maximum number of elements
constexpr size_t N_H = 252;  // maximum number of hyperplanes

constexpr size_t bnml = 252;          // C(10, 5)
constexpr size_t bnml_nm1 = 126;      // C(9, 5)
constexpr size_t bnml_nm1_rm1 = 126;  // C(9, 4)

inline unsigned char
    P[30240][252];  // representatives (an ordered choice of 5 first elements)
inline unsigned char T[120][252];  // relative transpositions of representatives

// f[i] = (i - 1)!
constexpr size_t f[11] = {0, 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880};

// C_r[i] = C(n - i, r)
constexpr unsigned char C_r[12] = {252, 126, 56, 21, 6, 1, 0, 0, 0, 0, 0, 0};

inline unsigned char set_to_index[1024];  // set from C([n], r) to index
inline bitset<N> index_to_set[252];       // index to set from C([n], r)
inline vector<bitset<N>> R;               // for taboo_hyperplanes calculation

inline unsigned char r_set_to_j[252];     // colex position for check
inline size_t r_set_to_perm_reps[30240];  // all perm_reps, grouped by r_set

template <size_t N>
struct CoLexComparator {
    bool operator()(const bitset<N>& a, const bitset<N>& b) const {
        for (int i = N - 1; i >= 0; --i) {
            if (a[i] != b[i]) {
                return a[i] < b[i];
            }
        }
        return false;
    }
};

template <size_t N>
vector<bitset<N>> combinations(size_t n, size_t r) {
    // Produce C([n], r) in colex order
    if (r == 0 || n == r) {
        bitset<N> b;
        for (size_t i = 0; i < r; ++i) b.set(i);
        return {b};
    }
    vector<bitset<N>> result = combinations<N>(n - 1, r);
    for (bitset<N> b : combinations<N>(n - 1, r - 1)) {
        b.set(n - 1);
        result.push_back(b);
    }
    return result;
}

template <size_t N>
inline vector<bitset<N>> generate_minus_1_subsets(const bitset<N>& set,
                                                  const size_t& n) {
    unordered_set<bitset<N>> subsets;
    for (size_t i = 0; i < n; ++i) {
        if (set[i]) {
            bitset<N> tmp = set;
            tmp.reset(i);
            subsets.insert(tmp);
        }
    }
    return vector<bitset<N>>(subsets.begin(), subsets.end());
}

inline size_t factorial(size_t n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

inline size_t binomial(size_t n, size_t k) {
    if (k == 0 || k == n) return 1;
    size_t res = 1;
    for (size_t i = 0; i < k; ++i) {
        res = res * (n - i) / (i + 1);
    }
    return res;
}

inline void initialize_combinatorics() {
    // Initialize mappings between indices and sets
    size_t i = 0;
    for (bitset<N> C : combinations<N>(10, 5)) {
        index_to_set[i++] = C;
    }
    for (unsigned char i = 0; i < bnml; ++i) {
        set_to_index[index_to_set[i].to_ulong()] = i;
    }

    auto apply_perm = [&](vector<size_t> perm, size_t j) -> unsigned char {
        bitset<N> transformed_set;
        for (size_t k = 0; k < 10; ++k)
            if (index_to_set[j][k]) transformed_set.set(perm[k]);
        return set_to_index[transformed_set.to_ulong()];
    };

    vector<size_t> r_set_counts(bnml, 0);
    vector<size_t> perm(10);
    for (size_t i = 0; i < 10; ++i) perm[i] = i;
    i = 0;
    do {
        // Fill permutation array P with representatives
        if (i % 120 == 0) {
            for (size_t j = 0; j < bnml; ++j) {
                P[i / 120][j] = apply_perm(perm, j);
            }
        }

        // Fill permutation array T
        if (i / 120 == 0) {
            for (size_t j = 0; j < bnml; ++j) {
                T[i][j] = apply_perm(perm, j);
            }
        }

        bitset<N> first_r;
        for (size_t k = 0; k < 5; ++k) first_r.set(perm[k]);
        size_t r_set_idx = set_to_index[first_r.to_ulong()];
        bool rest_sorted = true;
        for (size_t k = 5; k + 1 < 10; ++k)
            if (perm[k] > perm[k + 1]) {
                rest_sorted = false;
                break;
            }
        if (rest_sorted) {
            size_t ind = r_set_counts[r_set_idx]++;
            r_set_to_perm_reps[r_set_idx * f[5 + 1] + ind] = i / 120;
            if (ind == 0) r_set_to_j[r_set_idx] = apply_perm(perm, 0);
        }
        ++i;
    } while (next_permutation(perm.begin(), perm.end()));

    // Initialize R: combos from C([9], 5) with 8
    R.clear();
    vector<bitset<N>> combos = combinations<N>(9, 5);
    for (const bitset<N>& combo : combos) {
        if (combo[8]) {
            R.push_back(combo);
        }
    }
}
