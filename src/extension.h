#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "matroid.h"

inline size_t fctrl;
inline size_t bnml;
inline std::vector<unsigned char> P;

// Check if matroid is canonical
inline bool is_canonical(const std::string& revlex) {
    const char* revlex_ptr = revlex.data();
    const unsigned char* perm = P.data();
    for (size_t i = 0; i < fctrl; ++i, perm += bnml) {
        for (size_t j = 0; j < bnml; ++j) {
            if (revlex_ptr[perm[j]] != revlex_ptr[j]) {
                if (revlex_ptr[j] == '*') {
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

constexpr size_t N_H = 256;  // maximum number of hyperplanes

inline int bnml_nm1;      // C(n - 1, r)
inline int bnml_nm1_rm1;  // C(n - 1, r - 1)
inline std::unordered_map<std::bitset<N>, unsigned char> set_to_index;

class Node {
   private:
    std::bitset<N_H> p_free;  // Available hyperplanes
    std::bitset<N_H> p_in;    // Selected hyperplanes
    std::bitset<N_H> l0;      // Lines with 0 hyperplanes
    std::bitset<N_H> l1;      // Lines with 1 hyperplane

   public:
    const Matroid* M;

    Node(const Matroid* M);
    Node(const Node& other);

    bool insert_plane(const int& p0);
    void remove_plane(const int& p0);
    int select_plane();
    std::vector<int> planes() const;
};

std::vector<Matroid> get_canonical_extensions(const Matroid& M);
Matroid extend_matroid_coloop(const Matroid& M);
