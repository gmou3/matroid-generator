#pragma once

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
    const unsigned char* perm = P.data();
    for (size_t i = 0; i < fctrl; ++i) {
        for (size_t j = 0; j < bnml; ++j) {
            // This inner loop typically breaks very fast (<3 iterations)
            // Thus, the (bnml x fctrl) layout of P is more cache-friendly
            if (revlex_ptr[perm[j * fctrl + i]] != revlex_ptr[j]) {
                if (revlex_ptr[j] == '*') {
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

    bool insert_plane(const int& p0);
    void remove_plane(const int& p0);
    int select_plane();
    vector<int> planes() const;
};

vector<Matroid> get_canonical_extensions(const Matroid& M);
Matroid extend_matroid_coloop(const Matroid& M);
