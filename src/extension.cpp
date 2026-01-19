#include <vector>

#include "combinatorics.h"
#include "extension.h"
#include "matroid.h"

using namespace std;

Node::Node(const Matroid* M) : M(M) {
    p_free.reset();
    for (int i = 0; i < M->hyperplanes.size(); ++i) {
        p_free.set(i);
    }
    p_in.reset();
    l0.reset();
    for (int i = 0; i < M->hyperlines.size(); ++i) {
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

bool Node::insert_plane(const int& p) {
    p_free.reset(p);
    p_in.set(p);

    vector<int> p_stack = {p};
    vector<int> l_stack;

    while (!p_stack.empty()) {
        // Process hyperplanes
        while (!p_stack.empty()) {
            int p = p_stack.back();
            p_stack.pop_back();

            for (int l : M->planes_to_lines[p]) {
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
            int l = l_stack.back();
            l_stack.pop_back();

            for (int pl : M->lines_to_planes[l]) {
                if (p_in[pl]) continue;

                if (p_free[pl]) {
                    p_free.reset(pl);
                    p_in.set(pl);
                    p_stack.push_back(pl);
                } else {
                    return false;  // forbidden
                }
            }
        }
    }
    return true;
}

void Node::remove_plane(const int& p) { p_free.reset(p); }

int Node::select_plane() {
    // Find first free plane
    for (int i = 0; i < M->hyperplanes.size(); ++i) {
        if (p_free[i]) return i;
    }
    return -1;
}

vector<int> Node::planes() const {
    vector<int> result;
    for (int i = 0; i < M->hyperplanes.size(); ++i) {
        if (p_in[i]) result.push_back(i);
    }
    return result;
}

string extend_matroid_LS(const Matroid& M, const vector<int>& linear_subclass) {
    // (r - 1)-sets whose extensions with n - 1 are to be marked '*'
    unordered_set<bitset<N>> target_sets;

    // Iterate over independent (r - 1)-sets and add to target_sets if they
    // aren't a subset of a hyperplane
    for (int i = 0; i < M.revlex.length(); ++i) {
        if (M.revlex[i] == '*') {
            for (const bitset<N>& S :
                 generate_minus_1_subsets(index_to_set[i], M.n)) {
                bool flag = true;
                for (const int& j : linear_subclass) {
                    if ((S & M.hyperplanes[j]) == S) {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    target_sets.insert(S);
                }
            }
        }
    }

    // Create extension result string of length C(n - 1, r - 1)
    string ext_res(bnml_nm1_rm1, '0');

    // Mark appropriate positions with '*'
    for (bitset<N> target_set : target_sets) {
        target_set.set(M.n);
        int index = set_to_index[target_set] - bnml_nm1;  // - C(n - 1, r)
        ext_res[index] = '*';
    }

    return ext_res;
}

bool dfs_search(Node& node, vector<Matroid>& canonical_extensions) {
    int p = node.select_plane();

    if (p < 0) {
        // No more free planes - this is a complete linear subclass
        string M_ext =
            node.M->revlex + extend_matroid_LS(*node.M, node.planes());
        if (is_canonical(M_ext)) {
            canonical_extensions.push_back(
                Matroid(node.M->n + 1, node.M->r, M_ext));
            return true;
        }
        return false;
    }

    // Exclude plane p (continue with remaining planes)
    Node exclude_node(node);
    exclude_node.remove_plane(p);
    bool exclusion_success = dfs_search(exclude_node, canonical_extensions);

    if (exclusion_success) {
        // Try including plane p
        Node include_node(node);
        if (include_node.insert_plane(p)) {
            dfs_search(include_node, canonical_extensions);
        }
    }

    return exclusion_success;
}

void traverse_linear_subclasses(const Matroid& M, bool exclude_taboo,
                                vector<Matroid>& canonical_extensions) {
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
    string revlex(binomial(M.n + 1, M.r + 1), '0');

    for (const bitset<N>& old_basis : M.bases(true)) {
        bitset<N> new_basis = old_basis;
        new_basis.set(M.n);
        revlex[set_to_index[new_basis]] = '*';
    }

    return Matroid(M.n + 1, M.r + 1, revlex);
}
