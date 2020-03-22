//------------------------------------------------------------------------------
//
// Definitions for intrusive inorder tree manager class.
//
// This file defines tree_t class that is parametrized by two kinds of payload.
// First one is leaf data that is attached to leaf nodes.
// Second one is branch data that is attached to internal nodes.
//
// The purpose of tree_t is to provide interface functions for managing
// nodes, like insert method that rebinds pointers in nodes.
// Thus, setters are not exposed to external user, only tree_t
// with same parameters L, B has access to internal state of nodes
// parametrized by L and B.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "semitree_iterator.h"
#include "semitree_nodes.h"

namespace semitree {
template <typename Leaf, typename Branch>
class tree_t : private branch_t<Leaf, Branch> {
public:
  using node_t = semitree::node_t<Leaf, Branch>;
  using branch_t = semitree::branch_t<Leaf, Branch>;

  using sibling_iterator_t = semitree::sibling_iterator_t<Leaf, Branch>;
  using const_sibling_iterator_t =
      semitree::const_sibling_iterator_t<Leaf, Branch>;
  using inorder_iterator_t = semitree::inorder_iterator_t<Leaf, Branch>;
  using const_inorder_iterator_t =
      semitree::const_inorder_iterator_t<Leaf, Branch>;

public:
  tree_t() = default;

  using branch_t::begin;
  using branch_t::empty;
  using branch_t::end;
  using branch_t::insert;

  // Iterators that denote inorder range of this tree.
  // Inorder range consists of all inserted nodes.
  auto inorder_end() { return inorder_iterator_t{this, true}; }
  auto inorder_begin() { return inorder_iterator_t{begin(), false}; }
  auto inorder_end() const { return const_inorder_iterator_t{this, true}; }
  auto inorder_begin() const {
    return const_inorder_iterator_t{begin(), false};
  }

  // Inorder insertion. Inserts BEFORE iterator.
  // Return value: 'it'.
  // Be careful passing inorder_begin from empty branch,
  // because returned iterator will point to inorder_end
  // since before insertion 'inorder_begin == inorder_end'.
  inorder_iterator_t insert(const_inorder_iterator_t it, node_t &n);
};

// Insert BEFORE iterator.
// Inorder insertion. Reduce it to sibling insertion.
// If node is parent and it is visited then insert after
// its last child. Else just convert iterator to sibling
// and insert in new location.
template <typename Leaf, typename Branch>
auto tree_t<Leaf, Branch>::insert(const_inorder_iterator_t it, node_t &n)
    -> inorder_iterator_t {
  if (it->ref.is_branch() && it->visited) {
    auto &parent =
        const_cast<branch_t &>(static_cast<const branch_t &>(it->ref));
    sibling_iterator_t pt = parent.insert(parent.end(), n);
    return inorder_iterator_t{pt, it->visited};
  }
  branch_t &parent = const_cast<branch_t &>(it->ref.get_parent());
  sibling_iterator_t pt = parent.insert(it->ref.get_sibling_iterator(), n);
  return inorder_iterator_t{pt, it->visited};
}
} // namespace semitree
