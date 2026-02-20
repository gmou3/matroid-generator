#pragma once

#include <algorithm>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

constexpr size_t N = 16;      // maximum number of elements
constexpr size_t N_H = 1024;  // maximum number of hyperplanes

inline size_t fctrl_m1;      // n! - 1
inline size_t bnml;          // C(n, r)
inline size_t bnml_nm1;      // C(n - 1, r)
inline size_t bnml_nm1_rm1;  // C(n - 1, r - 1)

inline vector<unsigned char> C_r;  // binomials choose r (shifted by one)

inline unsigned char set_to_index[65536];  // set from C([n], r) to index
inline vector<bitset<N>> index_to_set;     // index to set from C([n], r)
inline vector<bitset<N>>
    index_to_set_rm1;        // index to set from C([n - 1], r - 1)
inline vector<bitset<N>> R;  // for taboo_hyperplanes calculation

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

inline vector<vector<size_t>> permutations(const vector<size_t>& items) {
    vector<vector<size_t>> result;
    vector<size_t> perm = items;
    sort(perm.begin(), perm.end());

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

inline void initialize_combinatorics(size_t n, size_t r) {
    // Initialize factorial and binomial coefficients
    fctrl_m1 = factorial(n) - 1;
    bnml = binomial(n, r);
    bnml_nm1 = binomial(n - 1, r);
    bnml_nm1_rm1 = binomial(n - 1, r - 1);

    // Initialize index-to-set mappings
    index_to_set = combinations<N>(n, r);
    index_to_set_rm1 = combinations<N>(n - 1, r - 1);

    // Initialize set-to-index mapping
    for (size_t i = 0; i < bnml; ++i) {
        set_to_index[index_to_set[i].to_ulong()] =
            static_cast<unsigned char>(i);
    }

    // Initialize R: combos from C([n - 1], r) with n - 2
    R.clear();
    vector<bitset<N>> combos = combinations<N>(n - 1, r);
    for (const bitset<N>& combo : combos) {
        if (combo[n - 2]) {
            R.push_back(combo);
        }
    }

    C_r[0] = 0;
    for (size_t i = 0; i <= n; ++i) {
        C_r[i + 1] = static_cast<unsigned char>(binomial(i, r));
    }
}
