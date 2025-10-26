// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "tree.hpp"

#include "exception.hpp"


cbp::tree::node& cbp::tree::root() {
    this->validate_invariants();

    return this->nodes.front();
}
const cbp::tree::node& cbp::tree::root() const {
    this->validate_invariants();

    return this->nodes.front();
}

void cbp::tree::validate_invariants() const {
    if (this->nodes.empty()) throw cbp::exception{"Broken invariant: the tree does not have a root node"};
    if (this->nodes.front().depth) throw cbp::exception{"Broken invariant: root node of the tree has non-zero depth"};

    for (std::size_t i = 0; i < this->nodes.size() - 1; ++i) {
        const auto& curr_node = this->nodes[i];
        const auto& next_node = this->nodes[i + 1];

        if (curr_node.depth + 1 < next_node.depth)
            throw cbp::exception{
                "Broken invariant: tree nodes {} and {} have depths {} and {}, which breaks the node nesting", //
                i, i + 1, curr_node.depth, next_node.depth                                                      //
            };
    }
}