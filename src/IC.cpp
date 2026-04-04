#include <omp.h>

#include <cstdint>
#include <iostream>
#include <vector>

#include "combinatorics.h"
#include "file.h"
#include "matroid.h"

using namespace std;

int num_threads = 1;

vector<string> IC(uint16_t n, uint16_t r, bool top_level = true) {
    // Base cases
    if (n < r) {
        return {};
    } else if (r == 0 || n == r) {
        Matroid M(n, r, "*");
        if (top_level) output_matroid(M, 0, 0);
        return {M.colex};
    }

    if (top_level) {
        // These sizes suffice because the recursive calls are
        // (n - 1, r) and (n - 1, r - 1)
        P = new uint16_t[binomial(n, r) * factorial(r) * binomial(n, r)];
        T = new uint16_t[factorial(n - r) * binomial(n, r)];
        index_to_set.resize(binomial(n, r));
        f.resize(n + 1);
        C_r.resize(n + 2);
        r_set_to_j.resize(binomial(n, r));
        r_set_to_perm_reps.resize(binomial(n, r) * factorial(r));
    }

    // Recursive calls
    vector<string> IC_nm1 = IC(n - 1, r, false);
    vector<string> IC_nm1_rm1 = IC(n - 1, r - 1, false);

    // Initialize factorials, binomial coefficients,
    // mappings between indices and sets,
    // and fill permutation array of size n! * C(n, r)
    initialize_combinatorics(n, r);

    // Process IC_nm1
    vector<string> matroids;
    vector<vector<string>> local_matroids(!top_level ? IC_nm1.size() : 0);
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
#pragma omp for schedule(dynamic, 1) nowait
        for (size_t i = 0; i < IC_nm1.size(); ++i) {
            Matroid M(n - 1, r, IC_nm1[i]);
            // Iterate over all canonical extensions
            M.canonical_extensions([&](Matroid M_ext) {
                if (top_level)
                    output_matroid(M_ext, i, tid);
                else
                    local_matroids[i].push_back(M_ext.colex);
            });
        }
    }

    for (auto& v : local_matroids) {
        matroids.insert(matroids.end(), make_move_iterator(v.begin()),
                        make_move_iterator(v.end()));
    }

    // Process IC_nm1_rm1
    for (const string& colex : IC_nm1_rm1) {
        Matroid M(n - 1, r - 1, colex);
        Matroid M_ext = M.coloop_extension();
        if (top_level)
            output_matroid(M_ext, IC_nm1.size(), 0);
        else
            matroids.push_back(move(M_ext.colex));
    }

    return matroids;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 6) {
        cout << "Usage: " << argv[0]
             << " <n> <r> [<num_threads>] [--file] [--compressed-file]" << endl;
        return 1;
    }

    // Parse arguments
    uint16_t n = static_cast<uint16_t>(stoul(argv[1]));
    uint16_t r = static_cast<uint16_t>(stoul(argv[2]));
    for (int i = 3; i < argc; ++i) {
        if (string(argv[i]) == "--file") {
            to_file = true;
        } else if (string(argv[i]) == "--compressed-file") {
            to_file = true;
            use_compression = true;
        } else {
            num_threads = stoi(argv[i]);
        }
    }
    omp_set_num_threads(num_threads);

    if (to_file) open_files(n, r, num_threads);

    // Main IC call
    IC(n, r);

    if (to_file) merge_files();

    return 0;
}
