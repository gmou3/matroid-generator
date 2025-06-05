#pragma once

#include <algorithm>
#include <bitset>
#include <unordered_set>
#include <vector>

template <size_t N>
struct RevLexComparator {
    bool operator()(const std::bitset<N>& a, const std::bitset<N>& b) const {
        for (int i = N - 1; i >= 0; --i) {
            if (a[i] != b[i]) {
                return a[i] < b[i];
            }
        }
        return false;
    }
};

template <size_t N>
std::vector<std::bitset<N>> combinations(const std::vector<int>& items, int r) {
    std::vector<std::bitset<N>> result;
    std::vector<bool> selector(items.size());
    fill(selector.end() - r, selector.end(), true);

    do {
        std::bitset<N> combination;
        for (int i = 0; i < items.size(); ++i) {
            if (selector[i]) {
                combination.set(i);
            }
        }
        result.push_back(combination);
    } while (next_permutation(selector.begin(), selector.end()));

    return result;
}

inline std::vector<std::vector<int>> permutations(
    const std::vector<int>& items) {
    std::vector<std::vector<int>> result;
    std::vector<int> perm = items;
    sort(perm.begin(), perm.end());

    do {
        result.push_back(perm);
    } while (next_permutation(perm.begin(), perm.end()));

    return result;
}

template <size_t N>
inline std::vector<std::bitset<N>> generate_minus_1_subsets(
    const std::bitset<N>& set, const int& n) {
    std::unordered_set<std::bitset<N>> subsets;

    for (int i = 0; i < n; ++i) {
        if (set[i]) {
            std::bitset<N> tmp = set;
            tmp.reset(i);
            subsets.insert(tmp);
        }
    }

    return std::vector<std::bitset<N>>(subsets.begin(), subsets.end());
}

inline size_t factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

inline size_t binomial(int n, int k) {
    if (k == 0 || k == n) return 1;
    size_t res = 1;
    for (int i = 0; i < k; ++i) {
        res = res * (n - i) / (i + 1);
    }
    return res;
}
