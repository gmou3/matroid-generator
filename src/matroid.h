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
    string colex;
    mutable set<bitset<N>, CoLexComparator<N>> ind_sets_rm1;
    mutable vector<bitset<N>> hyperplanes;
    mutable unordered_set<bitset<N>> taboo_hyperplanes;
    mutable vector<bitset<N>> hyperlines;
    mutable vector<vector<unsigned char>> planes_to_lines;
    mutable vector<vector<unsigned char>> lines_to_planes;
    mutable unordered_map<bitset<N>, unsigned char> hyperplanes_index;
    mutable vector<vector<unsigned char>> hyperplanes_to_zeros;

    Matroid(const size_t& n, const size_t& r, const string& colex)
        : n(n), r(r), colex(colex) {}

    size_t rank(const bitset<N>& F) const;
    bitset<N> closure(const bitset<N>& F) const;
    void init_ind_sets_rm1() const;
    void init_hyperplanes() const;
    void init_taboo_hyperplanes() const;
    void init_hyperlines() const;

    Matroid coloop_extension() const {
        string colex(bnml, '0');
        // C(n, r) = C(n - 1, r - 1) + C(n - 1, r)
        for (size_t i = 0; i < bnml_nm1_rm1; ++i) {
            colex[bnml_nm1 + i] = this->colex[i];
        }
        return Matroid(this->n + 1, this->r + 1, colex);
    }

    template <typename F>
    void canonical_extensions(F on_extension) const;
};

#include "extension.h"  // here in order to avoid circular dependencies

template <typename F>
void Matroid::canonical_extensions(F on_extension) const {
    return get_canonical_extensions(*this, on_extension);
}
