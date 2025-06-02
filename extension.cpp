#include "combinatorics.h"
#include "extension.h"

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

void dfs_search(Node& node, vector<vector<int>>& linear_subclasses) {
    int p = node.select_plane();

    if (p < 0) {
        // No more free planes - this is a complete linear subclass
        linear_subclasses.push_back(node.planes());
        return;
    }

    // Try including plane p
    Node include_node(node);
    if (include_node.insert_plane(p)) {
        dfs_search(include_node, linear_subclasses);
    }

    // Try excluding plane p (continue with remaining planes)
    node.remove_plane(p);
    dfs_search(node, linear_subclasses);
}

vector<vector<int>> get_linear_subclasses(const Matroid& M,
                                          bool exclude_taboo) {
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

    vector<vector<int>> linear_subclasses;

    // Start DFS from the initial node
    dfs_search(first_node, linear_subclasses);

    return linear_subclasses;
}

Matroid extend_matroid_coloop(const Matroid& matroid) {
    string revlex(binomial(matroid.n + 1, matroid.r + 1), '0');

    for (const bitset<N>& old_basis : matroid.bases(true)) {
        bitset<N> new_basis = old_basis;
        new_basis.set(matroid.n);
        revlex[set_to_num[new_basis]] = '*';
    }

    return Matroid(matroid.n + 1, matroid.r + 1, revlex);
}

string extend_matroid_LS(const Matroid& matroid,
                         const vector<int>& linear_subclass) {
    // (r - 1)-sets whose extensions with n - 1 are to be marked '*'
    unordered_set<bitset<N>> target_sets;

    // Iterate over independent (r - 1)-sets and add to target_sets if they
    // aren't a subset of a hyperplane
    for (int i = 0; i < matroid.revlex.length(); ++i) {
        if (matroid.revlex[i] == '*') {
            for (const bitset<N>& S :
                 generate_minus_1_subsets(subsets[i], matroid.n)) {
                bool flag = true;
                for (const int& j : linear_subclass) {
                    if ((S & matroid.hyperplanes[j]) == S) {
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
    string ext_res(bnml_prev, '0');

    // Mark appropriate positions with '*'
    for (bitset<N> t_set : target_sets) {
        t_set.set(matroid.n);
        int index = set_to_num[t_set] - bnml_n_minus_1;  // - C(n - 1, r)
        ext_res[index] = '*';
    }

    return ext_res;
}
