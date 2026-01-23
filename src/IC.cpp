#include <omp.h>

#include <iostream>
#include <vector>

#include "combinatorics.h"
#include "extension.h"
#include "file.h"
#include "matroid.h"

using namespace std;

int num_threads;
bool to_file;
vector<string> f;  // file names for each thread

vector<Matroid> IC(int n, int r, bool top_level = true) {
    if (r < 0 || n < r) {
        return {};
    }

    if (top_level) {
        P.resize(factorial(n) * binomial(n, r));
        index_to_set.resize(binomial(n, r));
        index_to_set_rm1.resize(binomial(n - 1, r - 1));
    }

    vector<Matroid> matroids;
    if (r == 0 || n == r) {
        // Base cases
        Matroid M(n, r, "*");
        if (top_level)
            output_matroid(M, to_file, f[0], 0);
        else
            matroids.push_back(M);
        return matroids;
    }

    // Recursive calls
    vector<Matroid> IC_prev_1 = IC(n - 1, r, false);
    vector<Matroid> IC_prev_2 = IC(n - 1, r - 1, false);

    /* Initialize factorials, binomial coefficients,
     * mappings between indices and sets,
     * and fill permutation array of size n! * C(n, r)
     */
    initialize_combinatorics(n, r);

    // Process IC_prev_1
    vector<vector<Matroid>> local_matroids(IC_prev_1.size());
#pragma omp parallel
    {
#pragma omp for schedule(dynamic, 1) nowait
        for (size_t i = 0; i < IC_prev_1.size(); ++i) {
            const Matroid& M = IC_prev_1[i];
            // Iterate over all linear subclasses without taboo hyperplanes
            for (const Matroid& M_ext : M.canonical_extensions()) {
                if (top_level)
                    output_matroid(M_ext, to_file, f[omp_get_thread_num()], i);
                else
                    local_matroids[i].push_back(M_ext);
            }
        }
    }

    for (const auto& v : local_matroids) {
        matroids.insert(matroids.end(), v.begin(), v.end());
    }

    // Process IC_prev_2
    for (const Matroid& M : IC_prev_2) {
        Matroid M_ext = extend_matroid_coloop(M);
        if (top_level)
            output_matroid(M_ext, to_file, f[num_threads - 1],
                           IC_prev_1.size());
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

    int n = stoi(argv[1]);
    int r = stoi(argv[2]);
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

    if (to_file) f = open_files(n, r, num_threads);

    // Main IC call
    IC(n, r);

    if (to_file) mergesort_and_delete(f);

    return 0;
}
