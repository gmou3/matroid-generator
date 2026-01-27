#pragma once

#include <algorithm>
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

// Check if matroid is canonical
inline bool is_canonical(const string& revlex) {
    const char* revlex_ptr = revlex.data();
    vector<char> current(revlex.begin(), revlex.end());
    char* current_ptr = current.data();

    for (size_t i = 0; i < fctrl_m1; ++i) {
        const size_t offset = P[i] * bnml_nm2_rm1;
        const unsigned char* perm_to = &P_revlex_to[offset];
        const unsigned char* perm = &P_revlex[offset];

        for (size_t j = 0; j < bnml_nm2_rm1; ++j) {
            swap(current_ptr[perm_to[j]], current_ptr[perm[j]]);
        }

        if (lexicographical_compare(revlex_ptr, revlex_ptr + bnml, current_ptr,
                                    current_ptr + bnml)) {
            return false;
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
