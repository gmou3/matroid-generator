#include <cstdint>
#include <iostream>
#include <string>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <r> <n> <colex>" << endl;
        return 1;
    }

    uint16_t r = static_cast<uint16_t>(stoul(argv[1]));
    uint16_t n = static_cast<uint16_t>(stoul(argv[2]));
    const string colex = argv[3];
    const uint16_t np1 = n + 1;

    if (r == 0 || r > n || np1 > N || colex.size() != binomial(n, r)) {
        cerr << "Invalid matroid: expected 0 < r <= n < " << N
             << " and a colex string of length C(n, r)" << endl;
        return 1;
    }

    P = new uint16_t[binomial(np1, r) * factorial(r) * binomial(np1, r)];
    T = new uint16_t[factorial(np1 - r) * binomial(np1, r)];
    index_to_set.resize(binomial(np1, r));
    f.resize(np1 + 1);
    C_r.resize(np1 + 2);
    r_set_to_perm_reps.resize(binomial(np1, r) * factorial(r));
    initialize_combinatorics(np1, r);

    Matroid M(r, n, colex);
    M.canonical_extensions(
        [](const Matroid& extension) { cout << extension.colex << '\n'; });

    delete[] P;
    delete[] T;

    cout << string(binomial(n, r + 1), '0') + colex << '\n';

    return 0;
}
