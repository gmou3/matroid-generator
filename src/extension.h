#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

inline size_t permuted_index(const vector<size_t>& sigma,
                             const size_t original_index, const size_t r) {
    const bitset<N>& subset = index_to_set[original_index];
    size_t permuted_idx = 0;
    size_t position = 1;
    for (size_t i = 0; i < N; ++i) {
        if (subset[sigma[i]]) {
            permuted_idx += C[i * (r + 1) + position];
            if (++position > r) break;
        }
    }
    return permuted_idx;
}

// Check if matroid is canonical
inline bool is_canonical(const string& revlex, size_t r) {
    for (size_t i = 0; i < fctrl_m1; ++i) {
        for (size_t j = 0; j < bnml; ++j) {
            if (revlex[permuted_index(perms[i], j, r)] != revlex[j]) {
                if (revlex[j] == '*') {
                    return false;
                }
                break;
            }
        }
    }
    return true;
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
