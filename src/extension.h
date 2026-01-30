#pragma once

#include <bitset>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

inline bool dfs_canonical(const char* revlex, size_t* sigma,
                          unsigned char* used, size_t pos, size_t n,
                          size_t perm_id) {
    // sigma is viewed inversely (sigma[k] becomes k)
    // Check new determinable sets from partial sigma
    // Loop through positions C(pos - 1, r) to C(pos, r)
    for (size_t j = C_r[pos]; j < C_r[pos + 1]; ++j) {
        // This inner loop typically breaks very fast (<3 iterations)
        // Thus, the (bnml x f[n]) layout of P is more cache-friendly
        if (revlex[P[j * f[n] + perm_id]] != revlex[j]) {
            if (revlex[j] == '*') {
                return false;  // Not canonical
            }
            return true;  // Prune
        }
    }

    // Complete sigma checked
    if (pos == n) {
        return true;
    }

    // Build one more position in partial sigma
    size_t smaller = 0;
    for (size_t i = 0; i < n; ++i) {
        if (used[i]) continue;

        sigma[pos] = i;
        used[i] = 1;

        if (!dfs_canonical(revlex, sigma, used, pos + 1, n,
                           perm_id + smaller * f[n - 1 - pos])) {
            return false;
        }

        used[i] = 0;
        smaller++;
    }

    return true;
}

inline bool is_canonical(const string& revlex, size_t n) {
    // Fast check: check if revlex has all its 0s on the front
    size_t first_star = revlex.find('*');
    size_t last_zero = revlex.rfind('0');
    if (last_zero == string::npos || last_zero < first_star) {
        return true;
    }

    // Main check: traverse permutations using DFS
    size_t sigma[N];
    unsigned char used[N] = {0};

    return dfs_canonical(revlex.data(), sigma, used, 0, n, 0);
}

class Node {
   private:
    bitset<N_H> p_free;  // Available hyperplanes
    bitset<N_H> p_in;    // Selected hyperplanes
    bitset<N_H> l0;      // Lines with 0 hyperplanes
    bitset<N_H> l1;      // Lines with 1 hyperplane

   public:
    const Matroid* M;

    Node(const Matroid* M);
    Node(const Node& other);

    bool insert_plane(const size_t& p0);
    void remove_plane(const size_t& p0);
    size_t select_plane();
    vector<size_t> planes() const;
};

vector<Matroid> get_canonical_extensions(const Matroid& M);
Matroid extend_matroid_coloop(const Matroid& M);
