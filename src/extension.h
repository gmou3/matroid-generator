#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

inline bool dfs_canonical(const char* colex, size_t* sigma, unsigned char* used,
                          size_t pos, size_t n) {
    // sigma is viewed inversely (sigma[k] becomes k)
    // Check new determinable sets from partial sigma
    // Loop through positions C(pos - 1, r) to C(pos, r)
    for (size_t j = C_r[pos]; j < C_r[pos + 1]; ++j) {
        const bitset<N>& subset = index_to_set[j];

        bitset<N> permuted_subset;
        for (size_t k = 0; k < pos; ++k) {
            if (subset[k]) {
                permuted_subset[sigma[k]] = 1;
            }
        }

        const unsigned char& permuted_index =
            set_to_index[permuted_subset.to_ulong()];

        if (colex[permuted_index] != colex[j]) {
            if (colex[j] == '*') {
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
    for (size_t i = 0; i < n; ++i) {
        if (used[i]) continue;

        sigma[pos] = i;
        used[i] = 1;

        if (!dfs_canonical(colex, sigma, used, pos + 1, n)) {
            return false;
        }

        used[i] = 0;
    }

    return true;
}

inline bool is_canonical(const string& colex, size_t n) {
    // Fast check: check if colex has all its 0s on the front
    size_t first_star = colex.find('*');
    size_t last_zero = colex.rfind('0');
    if (last_zero == string::npos || last_zero < first_star) {
        return true;
    }

    // Main check: traverse permutations using DFS
    size_t sigma[N];
    unsigned char used[N] = {0};

    return dfs_canonical(colex.data(), sigma, used, 0, n);
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
