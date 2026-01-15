#include <omp.h>

#include <bitset>
#include <iostream>
#include <numeric>
#include <vector>

#include "combinatorics.h"
#include "extension.h"
#include "matroid.h"
#include "file.h"

using namespace std;

int num_threads;
bool output_to_file;

size_t fctrl;
size_t bnml;
vector<unsigned char> P;

// Check if matroid is canonical
bool is_canonical(const string& revlex) {
    for (size_t i = 0; i < fctrl; ++i) {
        size_t offset = i * bnml;
        for (size_t j = 0; j < bnml; ++j) {
            if (revlex[P[offset + j]] != revlex[j]) {
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
    if (r < 0 || n < r) {
        throw runtime_error("Ensure that r >= 0 and n >= r");
    }

    if (top_level) {
        P.resize(factorial(n) * binomial(n, r));
        index_to_set.resize(binomial(n, r));
        index_to_set_rm1.resize(binomial(n - 1, r - 1));
    }

    vector<Matroid> matroids;
    if (r == 0 || n == r) {
        // Base cases
        matroids.push_back(Matroid(n, r, "*"));
        if (top_level) {
            cout << "*" << endl;
        }
        return matroids;
    }

    // Recursive calls
    vector<Matroid> IC_prev_1 = IC(n - 1, r);
    vector<Matroid> IC_prev_2 = IC(n - 1, r - 1);

    // Initialize factorial and binomial coefficients
    fctrl = factorial(n);
    bnml = binomial(n, r);
    bnml_nm1 = binomial(n - 1, r);
    bnml_nm1_rm1 = binomial(n - 1, r - 1);

    // Initialize index-to-set mappings
    vector<int> range_n(n), range_nm1(n - 1);
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
    for (int i = 0; i < bnml; ++i) {
        set_to_index[index_to_set[i]] = i;
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
    vector<vector<int>> perms = permutations(range_n);
    for (size_t i = 0; i < fctrl; ++i) {
        for (size_t j = 0; j < bnml; ++j) {
            bitset<N> transformed_set;
            for (int k = 0; k < n; ++k) {
                if (index_to_set[j][k]) {
                    transformed_set.set(perms[i][k]);
                }
            }
            P[i * bnml + j] = set_to_index[transformed_set];
        }
    }

    vector<string> f;
    if (top_level && output_to_file) f = open_files(n, r, num_threads);

    // Process IC_prev_1
    vector<vector<Matroid>> local_matroids(num_threads);
#pragma omp parallel
    {
#pragma omp for schedule(dynamic)
        for (size_t i = 0; i < IC_prev_1.size(); ++i) {
            const Matroid& M = IC_prev_1[i];
            // Iterate over all linear subclasses without taboo hyperplanes
            for (const vector<int>& LS : M.linear_subclasses(true)) {
                string revlex_ext = M.revlex + extend_matroid_LS(M, LS);
                if (is_canonical(revlex_ext)) {
                    Matroid M_ext(n, r, revlex_ext);
                    if (top_level) {
                        if (output_to_file) {
                            write_revlex_to_file(f[omp_get_thread_num()], M_ext);
                        } else {
#pragma omp critical(io)
                            {
                                cout << M_ext.revlex << endl;
                            }
                        }
                    }
                    local_matroids[omp_get_thread_num()].push_back(M_ext);
                }
            }
        }
    }

    for (const auto& v : local_matroids) {
        matroids.insert(matroids.end(), v.begin(), v.end());
    }

    // Process IC_prev_2
    for (const Matroid& M : IC_prev_2) {
        Matroid M_ext = extend_matroid_coloop(M);
        if (top_level) {
            if (output_to_file) {
                write_revlex_to_file(f[num_threads - 1], M_ext);
            } else {
                cout << M_ext.revlex << endl;
            }
        }
        matroids.push_back(M_ext);
    }

    if (top_level && output_to_file) merge_and_delete(f);

    return matroids;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 5) {
        cout << "Usage: " << argv[0] << " <n> <r> [<num_threads>] [--file]" << endl;
        return 1;
    }

    int n = stoi(argv[1]);
    int r = stoi(argv[2]);
    num_threads = 1;
    output_to_file = false;
    if (argc >= 4) {
        if (string(argv[3]) != "--file") {
            num_threads = stoi(argv[3]);
        } else {
            output_to_file = true;
        }
    }
    omp_set_num_threads(num_threads);
    if (argc == 5 && string(argv[4]) == "--file") {
        output_to_file = true;
    }

    try {
        vector<Matroid> matroids = IC(n, r, true);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
