#include <omp.h>

#include <iostream>
#include <vector>

#include "combinatorics.h"
#include "extension.h"
#include "file.h"
#include "matroid.h"

using namespace std;

int num_threads;

vector<Matroid> IC(size_t n, size_t r, bool top_level = true) {
    // Base cases
    if (n < r) {
        return {};
    } else if (r == 0 || n == r) {
        Matroid M(n, r, "*");
        if (top_level) output_matroid(M, 0);
        return {M};
    }

    if (top_level) {
        index_to_set.resize(binomial(n, r));
        index_to_set_rm1.resize(binomial(n - 1, r - 1));
        C.resize((n + 1) * (r + 1));
    }

    // Recursive calls
    vector<Matroid> IC_nm1 = IC(n - 1, r, false);
    vector<Matroid> IC_nm1_rm1 = IC(n - 1, r - 1, false);

    // Initialize factorials, binomial coefficients,
    // mappings between indices and sets,
    // and fill permutation array of size C(n, r) * n!
    initialize_combinatorics(n, r);

    // Process IC_nm1
    vector<Matroid> matroids;
    vector<vector<Matroid>> local_matroids(IC_nm1.size());
#pragma omp parallel
    {
#pragma omp for schedule(dynamic, 1) nowait
        for (size_t i = 0; i < IC_nm1.size(); ++i) {
            const Matroid& M = IC_nm1[i];
            // Iterate over all linear subclasses without taboo hyperplanes
            for (const Matroid& M_ext : M.canonical_extensions()) {
                if (top_level)
                    output_matroid(M_ext, i);
                else
                    local_matroids[i].push_back(M_ext);
            }
        }
    }

    for (const auto& v : local_matroids) {
        matroids.insert(matroids.end(), v.begin(), v.end());
    }

    // Process IC_nm1_rm1
    for (const Matroid& M : IC_nm1_rm1) {
        Matroid M_ext = extend_matroid_coloop(M);
        if (top_level)
            output_matroid(M_ext, IC_nm1.size());
        else
            matroids.push_back(M_ext);
    }

    return matroids;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 5) {
        cout << "Usage: " << argv[0] << " <n> <r> [<num_threads>] [--file]"
             << endl;
        return 1;
    }

    size_t n = stoul(argv[1]);
    size_t r = stoul(argv[2]);
    num_threads = 1;
    to_file = false;
    if (argc >= 4) {
        if (string(argv[3]) != "--file") {
            num_threads = stoi(argv[3]);
        } else {
            to_file = true;
        }
    }
    omp_set_num_threads(num_threads);
    if (argc == 5 && string(argv[4]) == "--file") {
        to_file = true;
    }

    if (to_file) open_files(n, r, num_threads);

    // Main IC call
    IC(n, r);

    if (to_file) merge_files();

    return 0;
}
