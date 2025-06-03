#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr size_t N = 16;  // maximum number of elements

inline std::vector<std::bitset<N>> index_to_set;  // index to set from C([n], r)
inline std::vector<std::bitset<N>>
    index_to_set_rm1;  // index to set from C([n - 1], r - 1)

inline std::vector<std::bitset<N>> R;  // for taboo_hyperplanes calculation

class Matroid {
   private:
    mutable std::unordered_map<std::bitset<N>, int> rank_cache;
    mutable std::unordered_map<std::bitset<N>, std::bitset<N>> closure_cache;

   public:
    int r;
    int n;
    std::string revlex;
    mutable std::vector<std::bitset<N>> hyperplanes;
    mutable std::unordered_set<std::bitset<N>> taboo_hyperplanes;
    mutable std::vector<std::bitset<N>> hyperlines;
    mutable std::vector<std::vector<int>> planes_to_lines;
    mutable std::vector<std::vector<int>> lines_to_planes;
    mutable std::unordered_map<std::bitset<N>, int> hyperplanes_index;

    Matroid(const int& n, const int& r, const std::string& revlex)
        : n(n), r(r), revlex(revlex) {}

    int rank(const std::bitset<N>& F) const;
    std::bitset<N> closure(const std::bitset<N>& F) const;
    std::vector<std::bitset<N>> bases(const bool& prev) const;
    void init_hyperplanes() const;
    void init_taboo_hyperplanes(const std::vector<std::bitset<N>>& R) const;
    void init_hyperlines() const;
    std::vector<std::vector<int>> linear_subclasses(bool exclude_taboo) const;
};
