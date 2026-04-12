#pragma once

#include <bitset>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

inline bool dfs_canonical(const char* colex, const size_t unset,
                          const uint16_t* P_row, const uint16_t* T_row) {
    // The variable `unset` stores the number of undetermined positions at the
    // end of the current partial permutation sigma. sigma is viewed inversely
    // (sigma[k] becomes k).

    // Check new determinable sets from partial sigma:
    // Loop through positions C(n - unset - 1, r) to C(n - unset, r).
    for (size_t j = C_r[unset + 1]; j < C_r[unset]; ++j) {
        if (colex[P_row[T_row[j]]] != colex[j]) {
            if (colex[j] == '*') {
                return false;  // Not canonical
            }
            return true;  // Prune
        }
    }

    // Complete sigma checked
    if (unset == 0) {
        return true;
    }

    // Build one more position in partial sigma
    for (size_t i = 0; i < unset; ++i) {
        // Increment perm_id by (unset - 1)! as we skip a smaller element
        if (!dfs_canonical(colex, unset - 1, P_row,
                           T_row + i * f[unset] * bnml)) {
            return false;
        }
    }

    return true;
}

inline bool is_canonical(const string& colex, size_t n, size_t r) {
    // Fast check: check if colex has all its 0s on the front
    size_t first_star = colex.find('*');
    size_t last_zero = colex.rfind('0');
    if (last_zero == string::npos || last_zero < first_star) {
        return true;
    }

    // Main check: traverse (partial) permutations using DFS
    for (size_t r_set_idx = 0; r_set_idx < bnml; ++r_set_idx) {
        if (colex[r_set_to_j[r_set_idx]] != '0') {
            continue;
        }
        for (size_t i = 0; i < f[r + 1]; ++i) {
            size_t perm_rep = r_set_to_perm_reps[r_set_idx * f[r + 1] + i];
            uint16_t* P_row = P + perm_rep * bnml;
            for (size_t j = 0; j < n - r; ++j) {
                if (!dfs_canonical(colex.data(), n - r - 1, P_row,
                                   T + j * f[n - r] * bnml)) {
                    return false;
                }
            }
        }
    }

    return true;
}

inline string extend_matroid_LS(const Matroid& M,
                                const vector<size_t>& linear_subclass,
                                const string& base_colex_ext) {
    // Mark appropriate positions with '0'
    string colex_ext = base_colex_ext;
    for (const size_t& j : linear_subclass) {
        for (const uint16_t& pos : M.hyperplanes_to_zeros[j]) {
            colex_ext[pos] = '0';
        }
    }
    return colex_ext;
}

inline size_t pop_back(vector<size_t>& v) {
    size_t val = v.back();
    v.pop_back();
    return val;
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

    bool insert_plane(const size_t& p0);
    void remove_plane(const size_t& p0);
    size_t select_plane();
    vector<size_t> planes() const;
};

inline Node::Node(const Matroid* M) : M(M) {
    p_free.reset();
    for (size_t i = 0; i < M->hyperplanes.size(); ++i) {
        p_free.set(i);
    }
    p_in.reset();
    l0.reset();
    for (size_t i = 0; i < M->hyperlines.size(); ++i) {
        l0.set(i);
    }
    l1.reset();
}

inline Node::Node(const Node& other)
    : p_free(other.p_free),
      p_in(other.p_in),
      l0(other.l0),
      l1(other.l1),
      M(other.M) {}

inline bool Node::insert_plane(const size_t& p) {
    vector<size_t> p_stack = {p};
    vector<size_t> l_stack;
    p_free.reset(p);
    p_in.set(p);
    while (!p_stack.empty()) {
        // Process hyperplanes
        while (!p_stack.empty()) {
            size_t p = pop_back(p_stack);
            for (size_t l : M->planes_to_lines[p]) {
                if (l0[l]) {
                    l0.reset(l);
                    l1.set(l);
                } else if (l1[l]) {
                    l1.reset(l);
                    l_stack.push_back(l);
                }
            }
        }
        // Process hyperlines
        while (!l_stack.empty()) {
            size_t l = pop_back(l_stack);
            for (size_t pl : M->lines_to_planes[l]) {
                if (p_in[pl]) continue;
                if (p_free[pl]) {
                    p_free.reset(pl);
                    p_in.set(pl);
                    p_stack.push_back(pl);
                } else {
                    return false;
                }
            }
        }
    }
    return true;
}

inline void Node::remove_plane(const size_t& p) {
    vector<size_t> p_stack = {p};
    vector<size_t> l_stack;
    p_free.reset(p);
    while (!p_stack.empty()) {
        // Process hyperplanes
        while (!p_stack.empty()) {
            size_t p = pop_back(p_stack);
            for (size_t l : M->planes_to_lines[p]) {
                if (l1[l]) {
                    l_stack.push_back(l);
                }
            }
        }
        // Process hyperlines
        while (!l_stack.empty()) {
            size_t l = pop_back(l_stack);
            for (size_t pl : M->lines_to_planes[l]) {
                if (p_free[pl]) {
                    p_free.reset(pl);
                    p_stack.push_back(pl);
                }
            }
        }
    }
}

inline size_t Node::select_plane() {
    // Find first free plane
    return p_free._Find_first();
}

inline vector<size_t> Node::planes() const {
    vector<size_t> result;
    for (size_t i = 0; i < M->hyperplanes.size(); ++i) {
        if (p_in[i]) result.push_back(i);
    }
    return result;
}

template <typename F>
bool dfs_search(Node& node, const string& base_colex_ext,
                const uint16_t& last_zero, F& on_extension) {
    size_t p = node.select_plane();

    if (p == N_H) {
        // No more free planes - this is a complete linear subclass
        string M_ext = node.M->colex + extend_matroid_LS(*node.M, node.planes(),
                                                         base_colex_ext);
        if (is_canonical(M_ext, node.M->n + 1, node.M->r)) {
            on_extension(Matroid(node.M->n + 1, node.M->r, M_ext));
            return true;
        }
        return false;
    }

    // Exclude plane p (continue with remaining planes)
    Node exclude_node(node);
    exclude_node.remove_plane(p);
    bool exclusion_success =
        dfs_search(exclude_node, base_colex_ext, last_zero, on_extension);

    // If exclusion failed, and p adds zeros after the current last zero, we can
    // skip checking the inclusion (guaranteed non-canonical)
    if (exclusion_success || last_zero > node.M->hyperplanes_to_zeros[p][0]) {
        // Try including plane p
        Node include_node(node);
        if (include_node.insert_plane(p)) {
            size_t pos = extend_matroid_LS(*node.M, include_node.planes(),
                                           base_colex_ext)
                             .rfind('0');
            uint16_t new_last_zero =
                (pos != std::string::npos) ? (uint16_t)pos : 0;
            dfs_search(include_node, base_colex_ext, new_last_zero,
                       on_extension);
        }
    }

    return exclusion_success;
}

template <typename F>
void traverse_linear_subclasses(const Matroid& M, bool exclude_taboo,
                                F& on_extension) {
    M.init_ind_sets_rm1();
    M.init_hyperplanes();
    M.init_hyperlines();

    // Create initial node
    Node first_node(&M);

    if (exclude_taboo) {
        M.init_taboo_hyperplanes();
        // Remove taboo hyperplanes
        for (bitset<N> T : M.taboo_hyperplanes) {
            first_node.remove_plane(M.hyperplanes_index[T]);
        }
    }

    // Create base colex extension string of length C(n - 1, r - 1)
    string base_colex_ext(bnml_nm1_rm1, '0');
    for (bitset<N> I : M.ind_sets_rm1) {
        I.set(M.n);
        base_colex_ext[set_to_index[I.to_ulong()] - bnml_nm1] = '*';
    }
    size_t pos = base_colex_ext.rfind('0');
    uint16_t last_zero = (pos != std::string::npos) ? (uint16_t)pos : 0;

    // Start DFS from the initial node
    dfs_search(first_node, base_colex_ext, last_zero, on_extension);
}

template <typename F>
void get_canonical_extensions(const Matroid& M, F on_extension) {
    traverse_linear_subclasses(M, true, on_extension);
}
