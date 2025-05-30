#include <omp.h>

#include <algorithm>
#include <iostream>
#include <numeric>

#include "extension.h"
#include "matroid.h"

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

vector<bitset<N>> combinations(const vector<int>& items, int r) {
    vector<bitset<N>> result;
    vector<bool> selector(items.size());
    fill(selector.end() - r, selector.end(), true);

    do {
        bitset<N> combination;
        for (int i = 0; i < items.size(); ++i) {
            if (selector[i]) {
                combination.set(i);
            }
        }
        result.push_back(combination);
    } while (next_permutation(selector.begin(), selector.end()));

    return result;
}

vector<vector<int>> permutations(const vector<int>& items) {
    vector<vector<int>> result;
    vector<int> perm = items;
    sort(perm.begin(), perm.end());

    do {
        result.push_back(perm);
    } while (next_permutation(perm.begin(), perm.end()));

    return result;
}

// Check if matroid is canonical
bool is_canonical(string& revlex) {
    for (int i = 0; i < fctrl; ++i) {
        for (int j = 0; j < bnml; ++j) {
            if (revlex[P[i * bnml + j]] != revlex[j]) {
                if (revlex[j] == '*') {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

vector<Matroid> IC(int n, int r, bool top_level = false) {
    vector<Matroid> matroids;
    if (r == 0) {
        matroids.push_back(Matroid(n, r, "*"));
        if (top_level) {
            cout << "*" << endl;
        }
        return matroids;
    }

    if (n == r) {
        if (r == 1)
            matroids.push_back(Matroid(n, r, "*"));
        else
            matroids.push_back(Matroid(n, r, "*"));
        if (top_level) {
            cout << "*" << endl;
        }
        return matroids;
    }

    if (top_level) {
        initialize_factorial(n);
        initialize_binomial(n);
    }

    // Recursive calls
    vector<Matroid> IC_prev_1 = IC(n - 1, r);
    vector<Matroid> IC_prev_2 = IC(n - 1, r - 1);

    fctrl = factorial[n];
    bnml = binomial[n][r];

    vector<int> range_n(n);
    iota(range_n.begin(), range_n.end(), 0);
    vector<bitset<N>> all_subsets_vec = combinations(range_n, r);
    sort(all_subsets_vec.begin(), all_subsets_vec.end(), RevLexComparator());
    subsets.clear();
    set_to_num.clear();
    for (int i = 0; i < all_subsets_vec.size(); ++i) {
        bitset<N> s = all_subsets_vec[i];
        subsets.push_back(s);
        set_to_num[s] = i;
    }
    vector<int> range_n_minus_1(n - 1);
    iota(range_n_minus_1.begin(), range_n_minus_1.end(), 0);
    vector<bitset<N>> all_subsets_prev_vec =
        combinations(range_n_minus_1, r - 1);
    sort(all_subsets_prev_vec.begin(), all_subsets_prev_vec.end(),
         RevLexComparator());
    subsets_prev.clear();
    for (int i = 0; i < all_subsets_prev_vec.size(); ++i) {
        bitset<N> s = all_subsets_prev_vec[i];
        subsets_prev.push_back(s);
    }

    // Initialize R: combinations of range(n - 1) choose r with n - 2
    R.clear();
    iota(range_n_minus_1.begin(), range_n_minus_1.end(), 0);
    vector<bitset<N>> all_combinations = combinations(range_n_minus_1, r);
    for (const bitset<N>& combo : all_combinations) {
        if (combo[n - 2]) {
            R.push_back(combo);
        }
    }
    // Sort R by reverse lexicographic order
    sort(R.begin(), R.end(), RevLexComparator());

    P = (unsigned char*)realloc(P, fctrl * bnml * sizeof(unsigned char));
    if (!P) {
        throw bad_alloc();
    }

    // Fill permutation array P
    vector<vector<int>> all_perms = permutations(range_n);
    for (int i = 0; i < all_perms.size(); ++i) {
        const vector<int>& p = all_perms[i];
        for (int j = 0; j < subsets.size(); ++j) {
            const bitset<N>& S = subsets[j];
            bitset<N> transformed_set;
            for (int k = 0; k < n; ++k) {
                if (S[k]) {
                    transformed_set.set(p[k]);
                }
            }
            P[i * bnml + j] = set_to_num[transformed_set];
        }
    }

    // Process IC_prev_1
#pragma omp parallel
    {
        vector<Matroid> local_matroids;
#pragma omp for schedule(dynamic) nowait
        for (int i = 0; i < IC_prev_1.size(); ++i) {
            const Matroid& M = IC_prev_1[i];
            // Iterate over all linear subclasses without taboo hyperplanes
            for (const vector<int>& LS : M.linear_subclasses(true)) {
                string revlex_ext = M.revlex + extend_matroid_LS(M, LS);
                if (is_canonical(revlex_ext)) {
                    Matroid M_ext(n, r, revlex_ext);
                    local_matroids.push_back(M_ext);
                    if (top_level) {
#pragma omp critical(cnt_io)
                        {
                            cout << M_ext.revlex << endl;
                        }
                    }
                }
            }
        }

#pragma omp critical(matroids_merge)
        {
            matroids.insert(matroids.end(), local_matroids.begin(),
                            local_matroids.end());
        }
    }

    // Process IC_prev_2
    for (const Matroid& M : IC_prev_2) {
        Matroid M_ext = extend_matroid_coloop(M);
        matroids.push_back(M_ext);
        if (top_level) {
            cout << M_ext.revlex << endl;
        }
    }

    return matroids;
}

// Clean up function
void cleanup() {
    if (P) {
        free(P);
        P = nullptr;
    }
    if (factorial) {
        free(factorial);
        factorial = nullptr;
    }
    if (binomial) {
        free(binomial);
        binomial = nullptr;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3 and argc != 4) {
        cout << "Usage: " << argv[0] << " <n> <r> [<num_threads>]" << endl;
        return 1;
    }

    int n = stoi(argv[1]);
    int r = stoi(argv[2]);
    int num_threads = omp_get_max_threads();
    if (argc == 4) {
        num_threads = stoi(argv[3]);
    }
    omp_set_num_threads(num_threads);

    try {
        vector<Matroid> matroids = IC(n, r, true);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        cleanup();
        return 1;
    }

    cleanup();
    return 0;
}
