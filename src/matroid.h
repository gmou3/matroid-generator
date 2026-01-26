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
    mutable unordered_map<bitset<N>, size_t> rank_cache;
    mutable unordered_map<bitset<N>, bitset<N>> closure_cache;

   public:
    size_t n;
    size_t r;
    string revlex;
    mutable set<bitset<N>, RevLexComparator<N>> ind_sets_rm1;
    mutable vector<bitset<N>> hyperplanes;
    mutable unordered_set<bitset<N>> taboo_hyperplanes;
    mutable vector<bitset<N>> hyperlines;
    mutable vector<vector<size_t>> planes_to_lines;
    mutable vector<vector<size_t>> lines_to_planes;
    mutable unordered_map<bitset<N>, size_t> hyperplanes_index;

    Matroid(const size_t& n, const size_t& r, const string& revlex)
        : n(n), r(r), revlex(revlex) {}

    size_t rank(const bitset<N>& F) const;
    bitset<N> closure(const bitset<N>& F) const;
    void init_ind_sets_rm1() const;
    void init_hyperplanes() const;
    void init_taboo_hyperplanes(const vector<bitset<N>>& R) const;
    void init_hyperlines() const;
    vector<Matroid> canonical_extensions() const;
};
