#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

constexpr uint16_t N = 16;      // maximum number of elements
constexpr uint16_t N_H = 1024;  // maximum number of hyperplanes

inline uint16_t bnml;          // C(n, r)
inline uint16_t bnml_nm1;      // C(n - 1, r)
inline uint16_t bnml_nm1_rm1;  // C(n - 1, r - 1)

inline uint16_t* P;  // representatives (an ordered choice of
                     // the first r elements)
inline uint16_t* T;  // relative transpositions of representatives (action
                     // on colex of the order of the rest n - r elements)

inline vector<size_t> f;      // factorials (shifted by one)
inline vector<uint16_t> C_r;  // binomials choose r (reversed)

inline uint16_t set_to_index[1 << N];   // set from C([n], r) to index
inline vector<bitset<N>> index_to_set;  // index to set from C([n], r)

inline vector<size_t> r_set_to_perm_reps;  // all perm reps, grouped by r-set

template <uint16_t N>
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

template <uint16_t N>
vector<bitset<N>> combinations(uint16_t n, uint16_t r) {
    // Produce C([n], r) in colex order
    if (r == 0 || n == r) {
        bitset<N> b;
        for (uint16_t i = 0; i < r; ++i) b.set(i);
        return {b};
    }
    vector<bitset<N>> result = combinations<N>(n - 1, r);
    for (bitset<N> b : combinations<N>(n - 1, r - 1)) {
        b.set(n - 1);
        result.push_back(b);
    }
    return result;
}

template <uint16_t N>
inline vector<bitset<N>> generate_minus_1_subsets(const bitset<N>& set,
                                                  const uint16_t& n) {
    unordered_set<bitset<N>> subsets;
    for (uint16_t i = 0; i < n; ++i) {
        if (set[i]) {
            bitset<N> tmp = set;
            tmp.reset(i);
            subsets.insert(tmp);
        }
    }
    return vector<bitset<N>>(subsets.begin(), subsets.end());
}

inline size_t factorial(uint16_t n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

inline uint16_t binomial(size_t n, size_t k) {
    if (k == 0 || k == n) return 1;
    size_t res = 1;
    for (size_t i = 0; i < k; ++i) {
        res = res * (n - i) / (i + 1);
    }
    return static_cast<uint16_t>(res);
}

inline void initialize_combinatorics(uint16_t n, uint16_t r) {
    // Initialize factorial array
    for (uint16_t i = 1; i <= n; ++i) {
        f[i] = factorial(i - 1);
    }

    // Initialize binomial coefficients
    bnml = binomial(n, r);
    bnml_nm1 = binomial(n - 1, r);
    bnml_nm1_rm1 = binomial(n - 1, r - 1);
    C_r[n + 1] = 0;
    for (uint16_t i = 0; i <= n; ++i) {
        C_r[i] = binomial(n - i, r);
    }

    // Initialize mappings between indices and sets
    index_to_set = combinations<N>(n, r);
    for (uint16_t i = 0; i < bnml; ++i) {
        set_to_index[index_to_set[i].to_ulong()] = i;
    }

    auto apply_perm = [&](vector<uint16_t> perm, uint16_t j) -> uint16_t {
        bitset<N> transformed_set;
        for (size_t k = 0; k < n; ++k)
            if (index_to_set[j][k]) transformed_set.set(perm[k]);
        return set_to_index[transformed_set.to_ulong()];
    };

    // Fill permutation array P
    vector<size_t> r_set_counts(bnml, 0);
    vector<uint16_t> perm(n);
    for (uint16_t i = 0; i < n; ++i) perm[i] = i;
    size_t i = 0;
    do {
        // Fill representative
        if (i % f[n - r + 1] == 0) {
            for (uint16_t j = 0; j < bnml; ++j) {
                P[i / f[n - r + 1] * bnml + j] = apply_perm(perm, j);
            }
        }

        // Fill transposition array T
        if (i / f[n - r + 1] == 0) {
            for (uint16_t j = 0; j < bnml; ++j) {
                T[i * bnml + j] = apply_perm(perm, j);
            }
        }

        bitset<N> first_r;
        for (uint16_t k = 0; k < r; ++k) first_r.set(perm[k]);
        uint16_t r_set_idx = set_to_index[first_r.to_ulong()];
        bool rest_sorted = true;
        for (uint16_t k = r; k + 1 < n; ++k) {
            if (perm[k] > perm[k + 1]) {
                rest_sorted = false;
                break;
            }
        }
        if (rest_sorted) {
            size_t ind = r_set_counts[r_set_idx]++;
            r_set_to_perm_reps[r_set_idx * f[r + 1] + ind] = i / f[n - r + 1];
        }
        ++i;
    } while (next_permutation(perm.begin(), perm.end()));
}
