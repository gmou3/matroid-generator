#pragma once

#include <algorithm>
#include <bitset>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

constexpr size_t N = 16;     // maximum number of elements
constexpr size_t N_H = 256;  // maximum number of hyperplanes

inline size_t fctrl_m1;      // n! - 1
inline size_t bnml;          // C(n, r)
inline size_t bnml_nm1;      // C(n - 1, r)
inline size_t bnml_nm1_rm1;  // C(n - 1, r - 1)

inline vector<unsigned char> P;
inline unordered_map<bitset<N>, unsigned char>
    set_to_index;                       // set from C([n], r) to index
inline vector<bitset<N>> index_to_set;  // index to set from C([n], r)
inline vector<bitset<N>>
    index_to_set_rm1;        // index to set from C([n - 1], r - 1)
inline vector<bitset<N>> R;  // for taboo_hyperplanes calculation

template <size_t N>
struct RevLexComparator {
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
vector<bitset<N>> combinations(const vector<size_t>& items, size_t r) {
    vector<bitset<N>> result;
    vector<bool> selector(items.size());
    fill(selector.end() - r, selector.end(), true);

    do {
        bitset<N> combination;
        for (size_t i = 0; i < items.size(); ++i) {
            if (selector[i]) {
                combination.set(i);
            }
        }
        result.push_back(combination);
    } while (next_permutation(selector.begin(), selector.end()));

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
    vector<size_t> range_n(n), range_nm1(n - 1);
    iota(range_n.begin(), range_n.end(),
         0);  // Fill with [n] = {0, 1, ..., n - 1}
    iota(range_nm1.begin(), range_nm1.end(),
         0);  // Fill with [n - 1] = {0, 1, ..., n - 2}
    index_to_set = combinations<N>(range_n, r);
    index_to_set_rm1 = combinations<N>(range_nm1, r - 1);
    sort(index_to_set.begin(), index_to_set.end(), RevLexComparator<N>());
    sort(index_to_set_rm1.begin(), index_to_set_rm1.end(),
         RevLexComparator<N>());

    // Initialize set-to-index mapping
    set_to_index.clear();
    for (size_t i = 0; i < bnml; ++i) {
        set_to_index[index_to_set[i]] = static_cast<unsigned char>(i);
    }

    // Initialize R: combos from C([n - 1], r) with n - 2
    R.clear();
    vector<bitset<N>> combos = combinations<N>(range_nm1, r);
    for (const bitset<N>& combo : combos) {
        if (combo[n - 2]) {
            R.push_back(combo);
        }
    }
    sort(R.begin(), R.end(), RevLexComparator<N>());

    // Fill permutation array P
    // The (bnml x fctrl) layout is more cache-friendly
    // See the function `is_canonical` for an explanation
    vector<vector<size_t>> perms = permutations(range_n);
    for (size_t i = 0; i < fctrl_m1; ++i) {
        for (size_t j = 0; j < bnml; ++j) {
            bitset<N> transformed_set;
            for (size_t k = 0; k < n; ++k) {
                if (index_to_set[j][k]) {
                    // i + 1 to skip identity permutation
                    transformed_set.set(perms[i + 1][k]);
                }
            }
            P[j * fctrl_m1 + i] = set_to_index[transformed_set];
        }
    }
}
