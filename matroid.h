#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr size_t N = 16;

using namespace std;

inline int fctrl;
inline unsigned char bnml;
inline unsigned char* P = nullptr;

inline unordered_map<bitset<N>, unsigned char> set_to_num;
inline vector<bitset<N>> subsets;       // index to C([n], r)-subset
inline vector<bitset<N>> subsets_prev;  // index to C([n - 1], r - 1)-subset

inline vector<bitset<N>> R;  // for taboo_hyperplanes calculation

inline int* factorial;
inline int size_factorial = 0;
inline void initialize_factorial(int n) {
    int new_size = n + 1;
    if (new_size <= size_factorial) {
        return;
    }

    factorial = (int*)realloc(factorial, new_size * sizeof(int));
    if (!factorial) {
        throw bad_alloc();
    }

    int start_index = size_factorial;
    for (int i = start_index; i < new_size; ++i) {
        if (i == 0) {
            factorial[0] = 1;
        } else {
            factorial[i] = factorial[i - 1] * i;
        }
    }

    size_factorial = new_size;
}

inline int binomial_coefficient(int n, int k) {
    if (k > n || k < 0) return 0;
    if (k == 0 || k == n) return 1;

    int result = 1;
    for (int i = 0; i < k; ++i) {
        result = result * (n - i) / (i + 1);
    }

    return result;
}

inline int** binomial;
inline int size_binomial = 0;
inline void initialize_binomial(int n) {
    int new_size = n + 1;

    if (new_size <= size_binomial) {
        return;
    }

    binomial = (int**)realloc(binomial, new_size * sizeof(int*));
    if (!binomial) {
        throw bad_alloc();
    }

    for (int i = size_binomial; i < new_size; ++i) {
        binomial[i] = (int*)malloc((i + 1) * sizeof(int));
        if (!binomial[i]) {
            throw bad_alloc();
        }

        for (int j = 0; j <= i; ++j) {
            binomial[i][j] = binomial_coefficient(i, j);
        }
    }

    size_binomial = new_size;
}

inline vector<bitset<N>> generate_minus_1_subsets(const bitset<N>& set,
                                                  const int& n) {
    unordered_set<bitset<N>> subsets;

    for (int i = 0; i < n; ++i) {
        if (set[i]) {
            bitset<N> tmp = set;
            tmp.reset(i);
            subsets.insert(tmp);
        }
    }

    return vector<bitset<N>>(subsets.begin(), subsets.end());
}

class Matroid {
   private:
    mutable unordered_map<bitset<N>, int> rank_cache;
    mutable unordered_map<bitset<N>, bitset<N>> closure_cache;

   public:
    int r;
    int n;
    string revlex;
    mutable vector<bitset<N>> hyperplanes;
    mutable unordered_set<bitset<N>> taboo_hyperplanes;
    mutable vector<bitset<N>> hyperlines;
    mutable vector<vector<int>> planes_to_lines;
    mutable vector<vector<int>> lines_to_planes;
    mutable unordered_map<bitset<N>, int> hyperplanes_index;

    Matroid(const int& n, const int& r, const string& revlex)
        : n(n), r(r), revlex(revlex) {}

    int rank(const bitset<N>& F) const;
    bitset<N> closure(const bitset<N>& F) const;
    vector<bitset<N>> bases(const bool& prev) const;
    void init_hyperplanes() const;
    void init_taboo_hyperplanes(const vector<bitset<N>>& R) const;
    void init_hyperlines() const;
    vector<vector<int>> linear_subclasses(bool exclude_taboo) const;
};
