#pragma once

#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

inline size_t dfs_canonical(const char* colex, const size_t unset,
                            const unsigned char* P_row,
                            const unsigned char* T_row) {
    // The variable `unset` stores the number of undetermined positions at the
    // end of the current partial permutation sigma. sigma is viewed inversely
    // (sigma[k] becomes k).

    // Check new determinable sets from partial sigma:
    // Loop through positions C(n - unset - 1, r) to C(n - unset, r).
    for (size_t j = C_r[unset + 1]; j < C_r[unset]; ++j) {
        if (colex[P_row[T_row[j]]] != colex[j]) {
            if (colex[j] == '*') {
                return j;  // Not canonical
            }
            return bnml;  // Prune
        }
    }

    // Complete sigma checked
    if (unset == 0) {
        return bnml;
    }

    // Build one more position in partial sigma
    for (size_t i = 0; i < unset; ++i) {
        // Increment perm_id by (unset - 1)! as we skip a smaller element
        size_t j_fail =
            dfs_canonical(colex, unset - 1, P_row, T_row + i * f[unset] * bnml);
        if (j_fail != bnml) return j_fail;
    }

    return bnml;
}

inline size_t is_canonical(const char* colex) {
    // Return first detected position of failure ('*' -> '0'),
    // or bnml if no such position exists (canonical)
    // Main check: traverse (partial) permutations using DFS
    for (size_t r_set_ind = 0; r_set_ind < bnml; ++r_set_ind) {
        if (colex[r_set_ind] != '0') {
            continue;
        }
        for (size_t i = 0; i < f[R + 1]; ++i) {
            const size_t perm_rep =
                r_set_to_perm_reps[r_set_ind * f[R + 1] + i];
            const unsigned char* P_row = P[perm_rep];
            for (size_t j = 0; j < N - R; ++j) {
                size_t j_fail = dfs_canonical(colex, N - R - 1, P_row,
                                              T[0] + j * f[N - R] * bnml);
                if (j_fail != bnml) return j_fail;
            }
        }
    }

    return bnml;
}

inline size_t pop_first(bitset<N_H>& b) {
    size_t val = b._Find_first();
    b.reset(val);
    return val;
}

class Node {
   public:
    bitset<N_H> p_free;  // Available hyperplanes
    bitset<N_H> p_in;    // Selected hyperplanes
    bitset<N_H> l0;      // Lines with 0 hyperplanes
    bitset<N_H> l1;      // Lines with 1 hyperplane

    const Matroid* M;

    Node(const Matroid* M);
    Node(const Node& other);

    bool insert_plane(const size_t& p0);
    void remove_plane(const size_t& p0);
    size_t select_plane();
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
    bitset<N_H> p_stack;
    p_stack.set(p);
    bitset<N_H> l_stack;
    p_free.reset(p);
    p_in.set(p);
    while (p_stack.any()) {
        // Process hyperplanes
        while (p_stack.any()) {
            size_t p = pop_first(p_stack);
            for (size_t l : M->planes_to_lines[p]) {
                if (l0[l]) {
                    l0.reset(l);
                    l1.set(l);
                } else if (l1[l]) {
                    l1.reset(l);
                    l_stack.set(l);
                }
            }
        }
        // Process hyperlines
        while (l_stack.any()) {
            size_t l = pop_first(l_stack);
            for (size_t pl : M->lines_to_planes[l]) {
                if (p_in[pl]) continue;
                if (p_free[pl]) {
                    p_free.reset(pl);
                    p_in.set(pl);
                    p_stack.set(pl);
                } else {
                    return false;
                }
            }
        }
    }
    return true;
}

inline void Node::remove_plane(const size_t& p) {
    bitset<N_H> p_stack;
    p_stack.set(p);
    bitset<N_H> l_stack;
    p_free.reset(p);
    while (p_stack.any()) {
        // Process hyperplanes
        while (p_stack.any()) {
            size_t p = pop_first(p_stack);
            for (size_t l : M->planes_to_lines[p]) {
                if (l1[l]) {
                    l_stack.set(l);
                }
            }
        }
        // Process hyperlines
        while (l_stack.any()) {
            size_t l = pop_first(l_stack);
            for (size_t pl : M->lines_to_planes[l]) {
                if (p_free[pl]) {
                    p_free.reset(pl);
                    p_stack.set(pl);
                }
            }
        }
    }
}

inline string extend_matroid_LS(const Node& N, const string& base_colex_ext) {
    // Mark appropriate positions with '0'
    string colex_ext = base_colex_ext;
    for (size_t i = N.p_in._Find_first(); i < N_H; i = N.p_in._Find_next(i)) {
        for (const unsigned char& pos : N.M->hyperplanes_to_zeros[i]) {
            colex_ext[pos] = '0';
        }
    }
    return colex_ext;
}

template <typename F>
size_t dfs_search(Node& node, const string& base_colex_ext, F& on_extension) {
    // Find first free plane (ordered by first independent (r - 1)-subset)
    size_t p = node.p_free._Find_first();

    if (p == N_H) {
        // No more free planes - this is a complete linear subclass
        string M_ext = node.M->colex + extend_matroid_LS(node, base_colex_ext);
        size_t j_fail = is_canonical(M_ext.data());
        if (j_fail == bnml) {  // Canonical matroid
            on_extension(Matroid(N, R, M_ext));
        }
        return j_fail;
    }

    // Exclude plane p (continue with remaining planes)
    Node exclude_node(node);
    exclude_node.remove_plane(static_cast<size_t>(p));
    size_t exclusion_j_fail =
        dfs_search(exclude_node, base_colex_ext, on_extension);

    // If p adds zeros only after a current position of failure, we can skip
    // checking the inclusion branch (guaranteed non-canonical)
    if (exclusion_j_fail >=
        static_cast<size_t>(bnml_nm1 + node.M->hyperplanes_to_zeros[p][0])) {
        // Try including plane p
        Node include_node(node);
        if (include_node.insert_plane(p)) {
            dfs_search(include_node, base_colex_ext, on_extension);
        }
    }

    return exclusion_j_fail;
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

    // Start DFS from the initial node
    dfs_search(first_node, base_colex_ext, on_extension);
}

template <typename F>
void get_canonical_extensions(const Matroid& M, F on_extension) {
    traverse_linear_subclasses(M, true, on_extension);
}
