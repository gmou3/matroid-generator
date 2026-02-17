#include "extension.h"

#include <vector>

#include "combinatorics.h"
#include "matroid.h"

using namespace std;

size_t pop_back(vector<size_t>& v) {
    size_t val = v.back();
    v.pop_back();
    return val;
}

Node::Node(const Matroid* M) : M(M) {
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

Node::Node(const Node& other)
    : p_free(other.p_free),
      p_in(other.p_in),
      l0(other.l0),
      l1(other.l1),
      M(other.M) {}

bool Node::insert_plane(const size_t& p) {
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

void Node::remove_plane(const size_t& p) {
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

size_t Node::select_plane() {
    // Find first free plane
    for (size_t i = 0; i < M->hyperplanes.size(); ++i) {
        if (p_free[i]) return i;
    }
    return M->hyperplanes.size();
}

vector<size_t> Node::planes() const {
    vector<size_t> result;
    for (size_t i = 0; i < M->hyperplanes.size(); ++i) {
        if (p_in[i]) result.push_back(i);
    }
    return result;
}

string extend_matroid_LS(const Matroid& M,
                         const vector<size_t>& linear_subclass) {
    // (r - 1)-sets whose extensions with n - 1 are to be marked '*'
    vector<bitset<N>> target_sets;

    // Iterate over independent (r - 1)-sets and add to target_sets if they
    // aren't a subset of a hyperplane
    for (const bitset<N>& I : M.ind_sets_rm1) {
        bool flag = true;
        for (const size_t& j : linear_subclass) {
            if ((I & M.hyperplanes[j]) == I) {
                flag = false;
                break;
            }
        }
        if (flag) {
            target_sets.push_back(I);
        }
    }

    // Create extension result string of length C(n - 1, r - 1)
    string ext_string(bnml_nm1_rm1, '0');

    // Mark appropriate positions with '*'
    for (bitset<N> target_set : target_sets) {
        target_set.set(M.n);
        size_t index =
            set_to_index[target_set.to_ulong()] - bnml_nm1;  // - C(n - 1, r)
        ext_string[index] = '*';
    }

    return ext_string;
}

bool dfs_search(Node& node, vector<Matroid>& canonical_extensions) {
    size_t p = node.select_plane();

    if (p == node.M->hyperplanes.size()) {
        // No more free planes - this is a complete linear subclass
        string M_ext =
            node.M->revlex + extend_matroid_LS(*node.M, node.planes());
        if (is_canonical(M_ext, node.M->n + 1)) {
            canonical_extensions.push_back(
                Matroid(node.M->n + 1, node.M->r, M_ext));
            return true;
        }
        return false;
    }

    // Exclude plane p (continue with remaining planes)
    Node exclude_node(node);
    exclude_node.remove_plane(static_cast<size_t>(p));
    bool exclusion_success = dfs_search(exclude_node, canonical_extensions);

    if (exclusion_success) {
        // Try including plane p
        Node include_node(node);
        if (include_node.insert_plane(static_cast<size_t>(p))) {
            dfs_search(include_node, canonical_extensions);
        }
    }

    return exclusion_success;
}

void traverse_linear_subclasses(const Matroid& M, bool exclude_taboo,
                                vector<Matroid>& canonical_extensions) {
    M.init_ind_sets_rm1();
    M.init_hyperplanes();
    M.init_hyperlines();

    // Create initial node
    Node first_node(&M);

    if (exclude_taboo) {
        M.init_taboo_hyperplanes(R);
        // Remove taboo hyperplanes
        for (bitset<N> T : M.taboo_hyperplanes) {
            first_node.remove_plane(M.hyperplanes_index[T]);
        }
    }

    // Start DFS from the initial node
    dfs_search(first_node, canonical_extensions);
}

vector<Matroid> get_canonical_extensions(const Matroid& M) {
    vector<Matroid> canonical_extensions;
    traverse_linear_subclasses(M, true, canonical_extensions);
    return canonical_extensions;
}

Matroid extend_matroid_coloop(const Matroid& M) {
    string revlex(bnml, '0');

    // C(n, r) = C(n - 1, r - 1) + C(n - 1, r)
    for (size_t i = 0; i < bnml_nm1_rm1; ++i) {
        revlex[bnml_nm1 + i] = M.revlex[i];
    }

    return Matroid(M.n + 1, M.r + 1, revlex);
}
