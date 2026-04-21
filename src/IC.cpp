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
    if (argc > 1) {
        cout << "Hardcoded (10, 4)-matroid-generator.\n"
             << "Requires `seed-matroids/n09r03-4`.\n"
             << "Usage: " << argv[0] << "\n";
        return 1;
    }

    open_sz_file();

    // Get seed matroids
    string repo_root =
        fs::canonical(argv[0]).parent_path().parent_path().string();
    vector<string> IC_nm1 =
        read_lines(repo_root + "/seed-matroids/n09r04", 0, 190214);
    vector<string> IC_nm1_rm1 =
        read_lines(repo_root + "/seed-matroids/n09r03", 0, 1275);

    // Initialize mappings between indices and sets,
    // and fill permutation array of size n! * C(n, r)
    initialize_combinatorics();

    // Process IC_nm1
    for (size_t i = 0; i < 190214; ++i) {
        Matroid M(N - 1, R, IC_nm1[i]);
        // Iterate over all canonical extensions
        M.canonical_extensions([&](Matroid M_ext) { output_matroid(M_ext); });
    }

    // Process IC_nm1_rm1
    for (const string& colex : IC_nm1_rm1) {
        Matroid M(N - 1, R - 1, colex);
        Matroid M_ext = M.coloop_extension();
        output_matroid(M_ext);
    }

    close_sz_file();

    return 0;
}
