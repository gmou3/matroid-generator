#include "extension.h"

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

bool Node::insert_plane(const int& p0) {
    p_free.reset(p0);
    p_in.set(p0);

    vector<int> p_stack = {p0};
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

            for (int p : M->lines_to_planes[l]) {
                if (p_in[p]) continue;

                if (p_free[p]) {
                    p_free.reset(p);
                    p_in.set(p);
                    p_stack.push_back(p);
                } else {
                    return false;  // forbidden
                }
            }
        }
    }
    return true;
}

void Node::remove_plane(const int& p0) {
    p_free.reset(p0);
}

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
    vector<Node> nodes;
    nodes.push_back(first_node);

    while (!nodes.empty()) {
        Node node(nodes.back());
        nodes.pop_back();
        int p0 = node.select_plane();
        while (p0 >= 0) {
            Node node2(node);
            if (node2.insert_plane(p0)) {
                nodes.push_back(node2);
            }
            node.remove_plane(p0);
            p0 = node.select_plane();
        }

        linear_subclasses.push_back(node.planes());
    }

    return linear_subclasses;
}

Matroid extend_matroid_coloop(const Matroid& matroid) {
    string revlex(bnml, '0');

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
    int total_combinations = binomial[matroid.n][matroid.r - 1];
    string ext_res(total_combinations, '0');

    // Mark appropriate positions with '*'
    for (bitset<N> t_set : target_sets) {
        t_set.set(matroid.n);
        int index = set_to_num[t_set] - binomial[matroid.n][matroid.r];
        ext_res[index] = '*';
    }

    return ext_res;
}
