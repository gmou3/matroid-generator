#include "matroid.h"

#include <bitset>
#include <cstdint>
#include <set>
#include <vector>

#include "combinatorics.h"

using namespace std;

uint16_t Matroid::rank(const bitset<N>& F) const {
    auto it = rank_cache.find(F);
    if (it != rank_cache.end()) {
        return it->second;
    }

    uint16_t F_cnt = static_cast<uint16_t>(F.count());
    uint16_t max_rank = 0;
    for (uint16_t i = 0; i < colex.length(); ++i) {
        if (colex[i] == '*') {
            uint16_t cnt = static_cast<uint16_t>((F & index_to_set[i]).count());
            if (cnt > max_rank) {
                max_rank = cnt;
                if (cnt == F_cnt) {
                    break;
                }
            }
        }
    }

    rank_cache[F] = max_rank;
    return max_rank;
}

bitset<N> Matroid::closure(const bitset<N>& F) const {
    auto it = closure_cache.find(F);
    if (it != closure_cache.end()) {
        return it->second;
    }

    bitset<N> cl = F;
    uint16_t rank_F = rank(F);
    for (uint16_t i = 0; i < n; ++i) {
        if (!cl[i]) {
            cl.set(i);
            if (rank(cl) > rank_F) {
                cl.reset(i);
            }
        }
    }

    closure_cache[F] = cl;
    return cl;
}

// Independent (r - 1)-sets
void Matroid::init_ind_sets_rm1() const {
    for (uint16_t i = 0; i < colex.size(); ++i) {
        if (colex[i] == '*') {
            for (const bitset<N>& S :
                 generate_minus_1_subsets<N>(index_to_set[i], n)) {
                ind_sets_rm1.insert(S);
            }
        }
    }
}

// Flats of rank r - 1
void Matroid::init_hyperplanes() const {
    // Hyperplanes are ordered by the colex order of their colex-smallest
    // independent (r - 1)-set. This ensures the lexicographic order of the
    // final output.
    unordered_set<bitset<N>> H_unordered;
    for (const bitset<N>& I : ind_sets_rm1) {
        H_unordered.insert(closure(I));
    }
    hyperplanes.reserve(H_unordered.size());
    for (const auto& I : ind_sets_rm1) {
        vector<bitset<N>> to_add;
        for (const auto& H : H_unordered) {
            if ((I & H) == I) {
                to_add.push_back(H);
            }
        }
        for (const auto& H : to_add) {
            hyperplanes.push_back(H);
            H_unordered.erase(H);
        }
    }
    for (uint16_t i = 0; i < hyperplanes.size(); ++i) {
        hyperplanes_index[hyperplanes[i]] = i;
    }
    hyperplanes_to_zeros.resize(hyperplanes.size());
    for (bitset<N> I : ind_sets_rm1) {
        for (uint16_t i = 0; i < hyperplanes.size(); ++i) {
            if ((I & hyperplanes[i]) == I) {
                I.set(n);
                hyperplanes_to_zeros[i].push_back(set_to_index[I.to_ulong()] -
                                                  bnml_nm1);
            }
        }
    }
}

void Matroid::init_taboo_hyperplanes() const {
    // Prop. 1
    uint16_t mx = 0;
    for (const bitset<N>& H : hyperplanes) {
        if (mx < H.count()) {
            mx = static_cast<uint16_t>(H.count());
        }
    }
    for (const bitset<N>& H : hyperplanes) {
        if (H.count() == mx) {
            taboo_hyperplanes.insert(H);
        }
    }

    // Prop. 2
    for (const bitset<N> S : combinations<N>(n - 1, r - 1)) {
        bitset<N> SS = S;
        SS.set(n - 1);                // add n - 2
        if (rank(SS) < SS.count()) {  // dependent
            if (rank(S) < S.count()) {
                // forced '0' agreement
                continue;
            }
            break;
        }
        // forced '*' agreement
        taboo_hyperplanes.insert(closure(S));
    }
}

// Flats of rank r - 2
void Matroid::init_hyperlines() const {
    unordered_set<bitset<N>> res_set;
    for (const bitset<N>& H : hyperplanes) {
        for (const bitset<N>& T : hyperplanes) {
            bitset<N> intersection = H & T;
            if (intersection.count() + 2 >= r && rank(intersection) + 2 == r) {
                res_set.insert(intersection);
            }
        }
    }
    hyperlines.assign(res_set.begin(), res_set.end());
    planes_to_lines.resize(hyperplanes.size());
    lines_to_planes.resize(hyperlines.size());
    for (const bitset<N>& H : hyperplanes) {
        uint16_t cnt = 0;
        for (const bitset<N>& L : hyperlines) {
            if ((H & L) == L) {
                planes_to_lines[hyperplanes_index[H]].push_back(cnt);
                lines_to_planes[cnt].push_back(hyperplanes_index[H]);
            }
            cnt++;
        }
    }
}
