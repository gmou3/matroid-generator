#pragma once

#include <bitset>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

inline bool dfs_canonical(const char* revlex, const size_t unset,
                          const size_t perm_id) {
    // The variable `unset` stores the number of undetermined positions at the
    // end of the current partial permutation sigma. sigma is viewed inversely
    // (sigma[k] becomes k).

    // Check new determinable sets from partial sigma:
    // Loop through positions C(n - unset - 1, r) to C(n - unset, r).
    for (size_t j = C_r[unset + 1]; j < C_r[unset]; ++j) {
        if (revlex[P[perm_id * bnml + j]] != revlex[j]) {
            if (revlex[j] == '*') {
                return false;  // Not canonical
            }
            return true;  // Prune
        }
    }

    // Complete sigma checked
    if (unset == 0) {
        return true;
    }

    // Build one more position in partial sigma
    for (size_t i = 0; i < unset; ++i) {
        // Increment perm_id by (unset - 1)! as we skip a smaller element
        if (!dfs_canonical(revlex, unset - 1, perm_id + i * f[unset])) {
            return false;
        }
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

    // Main check: traverse (partial) permutations using DFS
    return dfs_canonical(revlex.data(), n, 0);
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
