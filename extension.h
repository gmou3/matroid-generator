#pragma once

#include <bitset>
#include <unordered_set>
#include <vector>

#include "matroid.h"

constexpr size_t N_H = 256;

using namespace std;

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

class LinearSubclasses {
   private:
    vector<bitset<N>> hyperlines_;
    vector<bitset<N>> hyperplanes_;
    vector<vector<int>> planes_on_line_;
    vector<vector<int>> lines_on_plane_;
    vector<int> forbidden_planes_;

   public:
    LinearSubclasses(const Matroid& M,
                     const unordered_set<bitset<N>>& taboo_hyperplanes);

    // Accessors
    int hyperplanes_count() const;
    int hyperlines_count() const;
    const vector<int>& lines_on_plane(const int& p) const;
    const vector<int>& planes_on_line(const int& l) const;
    const vector<int>& forbidden_planes() const;
};

vector<vector<int>> get_linear_subclasses(const Matroid& M, bool exclude_taboo);
Matroid extend_matroid_coloop(const Matroid& matroid);
string extend_matroid_LS(const Matroid& matroid,
                         const vector<int>& linear_subclass);
