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

inline unsigned char P[914457600];

// f[i] = (i - 1)!
constexpr size_t f[11] = {0, 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880};

// C_r[i] = C(n - i, r)
constexpr unsigned char C_r[12] = {252, 126, 56, 21, 6, 1, 0, 0, 0, 0, 0, 0};

inline unsigned char set_to_index[1024];  // set from C([n], r) to index
inline bitset<N> index_to_set[252];       // index to set from C([n], r)
inline vector<bitset<N>> R;               // for taboo_hyperplanes calculation

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

inline vector<vector<size_t>> permutations(size_t n) {
    vector<vector<size_t>> result;
    vector<size_t> perm(n);
    for (size_t i = 0; i < n; ++i) perm[i] = i;
    do {
        result.push_back(perm);
    } while (next_permutation(perm.begin(), perm.end()));
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

    // Fill permutation array P
    vector<vector<size_t>> perms = permutations(10);
    for (size_t i = 0; i < factorial(10); ++i) {
        for (size_t j = 0; j < bnml; ++j) {
            bitset<N> transformed_set;
            for (size_t k = 0; k < 10; ++k) {
                if (index_to_set[j][k]) {
                    transformed_set.set(perms[i][k]);
                }
            }
            P[i * bnml + j] = set_to_index[transformed_set.to_ulong()];
        }
    }

    // Initialize R: combos from C([9], 5) with 8
    R.clear();
    vector<bitset<N>> combos = combinations<N>(9, 5);
    for (const bitset<N>& combo : combos) {
        if (combo[8]) {
            R.push_back(combo);
        }
    }
}
