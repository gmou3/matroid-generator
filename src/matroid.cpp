#include <bitset>
#include <set>

#include "combinatorics.h"
#include "extension.h"
#include "matroid.h"

using namespace std;

int Matroid::rank(const bitset<N>& F) const {
    auto it = rank_cache.find(F);
    if (it != rank_cache.end()) {
        return it->second;
    }

    int F_cnt = F.count();
    int max_rank = -1;
    for (int i = 0; i < revlex.length(); ++i) {
        if (revlex[i] == '*') {
            int cnt = (F & index_to_set[i]).count();
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
    int rank_F = rank(F);
    for (int i = 0; i < n; ++i) {
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

vector<bitset<N>> Matroid::bases(const bool& prev = false) const {
    vector<bitset<N>> bases;
    vector<bitset<N>>& S = index_to_set;
    if (prev) {
        S = index_to_set_rm1;
    }
    for (int i = 0; i < revlex.length(); ++i) {
        if (revlex[i] == '*') {
            bases.push_back(S[i]);
        }
    }
    return bases;
}

// Flats of rank r - 1
void Matroid::init_hyperplanes() const {
    /*
     * Hyperplanes are ordered by the revlex order of their
     * revlex-smallest independent (r-1)-set.
     * This ensures the lexicographic order of the final output.
     */
    unordered_set<bitset<N>> H_unordered;
    set<bitset<N>, RevLexComparator<N>> ind_sets_rm1;
    for (int i = 0; i < revlex.size(); ++i) {
        if (revlex[i] == '*') {
            for (const bitset<N>& S :
                 generate_minus_1_subsets(index_to_set[i], n)) {
                H_unordered.insert(closure(S));
                ind_sets_rm1.insert(S);
            }
        }
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
    for (int i = 0; i < hyperplanes.size(); ++i) {
        hyperplanes_index[hyperplanes[i]] = i;
    }
}

void Matroid::init_taboo_hyperplanes(const vector<bitset<N>>& R) const {
    // Prop. 1
    int mx = 0;
    for (const bitset<N>& H : hyperplanes) {
        if (mx < H.count()) {
            mx = H.count();
        }
    }
    for (const bitset<N>& H : hyperplanes) {
        if (H.count() == mx) {
            taboo_hyperplanes.insert(H);
        }
    }

    // Prop. 2
    for (const bitset<N> S : R) {
        bitset<N> SS = S;
        SS.reset(n - 1);            // remove n - 2
        if (rank(S) < S.count()) {  // dependent
            if (rank(SS) < SS.count()) {
                // forced '0' agreement
                continue;
            }
            break;
        }
        // forced '*' agreement
        taboo_hyperplanes.insert(closure(SS));
    }
}

// Flats of rank r - 2
void Matroid::init_hyperlines() const {
    unordered_set<bitset<N>> res_set;
    for (const bitset<N>& H : hyperplanes) {
        for (const bitset<N>& T : hyperplanes) {
            bitset<N> intersection = H & T;
            if (intersection.count() >= r - 2 && rank(intersection) == r - 2) {
                res_set.insert(intersection);
            }
        }
    }
    hyperlines.assign(res_set.begin(), res_set.end());
    planes_to_lines.resize(hyperplanes.size());
    lines_to_planes.resize(hyperlines.size());
    for (const bitset<N>& H : hyperplanes) {
        int cnt = 0;
        for (const bitset<N>& L : hyperlines) {
            if ((H & L) == L) {
                planes_to_lines[hyperplanes_index[H]].push_back(cnt);
                lines_to_planes[cnt].push_back(hyperplanes_index[H]);
            }
            cnt++;
        }
    }
}

vector<Matroid> Matroid::canonical_extensions() const {
    return get_canonical_extensions(*this);
}
