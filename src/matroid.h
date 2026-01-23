#pragma once

#include <bitset>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"

using namespace std;

class Matroid {
   private:
    mutable unordered_map<bitset<N>, int> rank_cache;
    mutable unordered_map<bitset<N>, bitset<N>> closure_cache;

   public:
    int r;
    int n;
    string revlex;
    mutable set<bitset<N>, RevLexComparator<N>> ind_sets_rm1;
    mutable vector<bitset<N>> hyperplanes;
    mutable unordered_set<bitset<N>> taboo_hyperplanes;
    mutable vector<bitset<N>> hyperlines;
    mutable vector<vector<int>> planes_to_lines;
    mutable vector<vector<int>> lines_to_planes;
    mutable unordered_map<bitset<N>, int> hyperplanes_index;

    Matroid(const int& n, const int& r, const string& revlex)
        : n(n), r(r), revlex(revlex) {}

    int rank(const bitset<N>& F) const;
    bitset<N> closure(const bitset<N>& F) const;
    void init_ind_sets_rm1() const;
    void init_hyperplanes() const;
    void init_taboo_hyperplanes(const vector<bitset<N>>& R) const;
    void init_hyperlines() const;
    vector<Matroid> canonical_extensions() const;
};
