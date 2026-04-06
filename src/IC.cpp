#include <filesystem>
#include <iostream>
#include <vector>

#include "combinatorics.h"
#include "file.h"
#include "matroid.h"

using namespace std;
namespace fs = std::filesystem;

vector<string> read_lines(const string& path, size_t from, size_t to) {
    ifstream file(path);
    vector<string> result;
    string line;
    size_t i = 0;

    while (getline(file, line)) {
        if (i >= from && i < to) result.push_back(line);
        if (i >= to) break;
        ++i;
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        cout << "Hardcoded (10, 5)-matroid-generator.\n"
             << "Requires `seed-matroids/n09r04-5`.\n"
             << "Usage: " << argv[0] << " <left_lim> [<right_lim>]\n"
             << "The limits refer to seed matroid indices: 0 to 190214. If not "
                "provided, `right_lim = left_lim + 1`.\n"
             << "Any index `i` less than 190214 produces the canonical "
                "extensions of the `i`-th (9, 5)-matroid.\n"
             << "The 190214 upper limit is inclusive and produces the "
                "extensions by a coloop of all (9, 4)-matroids.\n";
        return 1;
    }

    // Parse arguments
    size_t left_lim = stoul(argv[1]);
    size_t right_lim = left_lim + 1;
    if (argc == 3) {
        right_lim = stoul(argv[2]);
    }

    if (left_lim >= right_lim || right_lim > 190215) {
        cerr << "Error: require 0 <= left_lim < right_lim <= 190215\n";
        return 1;
    }

    open_files(left_lim, right_lim);

    // Get seed matroids
    string repo_root =
        fs::canonical(argv[0]).parent_path().parent_path().string();
    vector<string> IC_nm1 =
        read_lines(repo_root + "/seed-matroids/n09r05", left_lim, right_lim);
    vector<string> IC_nm1_rm1;
    if (right_lim == 190215) {
        IC_nm1_rm1 = read_lines(repo_root + "/seed-matroids/n09r04", 0, 190214);
        right_lim--;
    }

    // Initialize mappings between indices and sets,
    // and fill permutation array of size 10! * C(10, 5)
    initialize_combinatorics();

    // Process IC_nm1
    for (size_t i = 0; i < right_lim - left_lim; ++i) {
        Matroid M(9, 5, IC_nm1[i]);
        // Iterate over all canonical extensions
        M.canonical_extensions(
            [&](Matroid M_ext) { output_matroid(M_ext, i); });
    }

    // Process IC_nm1_rm1
    for (const string& colex : IC_nm1_rm1) {
        Matroid M(9, 4, colex);
        Matroid M_ext = M.coloop_extension();
        output_matroid(M_ext, right_lim - left_lim);
    }

    close_files();

    return 0;
}
