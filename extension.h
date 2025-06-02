#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "matroid.h"

constexpr size_t N_H = 256;  // maximum number of hyperplanes

inline int bnml_n_minus_1;  // C(n - 1, r)
inline int bnml_prev;       // C(n - 1, r - 1)
inline std::unordered_map<std::bitset<N>, unsigned char> set_to_num;

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

std::vector<std::vector<int>> get_linear_subclasses(const Matroid& M,
                                                    bool exclude_taboo);
Matroid extend_matroid_coloop(const Matroid& matroid);
std::string extend_matroid_LS(const Matroid& matroid,
                              const std::vector<int>& linear_subclass);
