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
template <typename Leaf, typename Branch> class tree_t {
public:
  using node_t = semitree::node_t<Leaf, Branch>;
  using branch_t = semitree::branch_t<Leaf, Branch>;

  using sibling_iterator_t = semitree::sibling_iterator_t<Leaf, Branch>;
  using inorder_iterator_t = semitree::inorder_iterator_t<Leaf, Branch>;

public:
  tree_t() = default;

  // Simple list insertion. Inserts BEFORE iterator.
  sibling_iterator_t insert(sibling_iterator_t it, node_t &n);

  // Inorder insertion. Inserts BEFORE iterator.
  inorder_iterator_t insert(inorder_iterator_t it, node_t &n);
};

// Insert BEFORE iterator.
// Simple list insertion. Just rebind left and right pointers
// and set parent if it presents.
template <typename Leaf, typename Branch>
auto tree_t<Leaf, Branch>::insert(sibling_iterator_t it, node_t &n)
    -> sibling_iterator_t {
  if (it->has_parent()) {
    auto &parent = it->get_parent();
    n.set_parent(&parent);
    if (it == parent.begin())
      parent.set_firstchild(&n);
  }

  node_t &left = it->get_prev();
  left.set_next(&n);
  n.set_prev(&left);
  it->set_prev(&n);
  n.set_next(&*it);
  return it;
}

// Insert BEFORE iterator.
// Inorder insertion. Reduce it to sibling insertion.
// If node is parent and it is visited then insert after
// its last child. Else just convert iterator to sibling
// and insert in new location.
template <typename Leaf, typename Branch>
auto tree_t<Leaf, Branch>::insert(inorder_iterator_t it, node_t &n)
    -> inorder_iterator_t {
  sibling_iterator_t ipt;
  if (it->ref.is_branch() && it->visited) {
    auto &parent = static_cast<branch_t &>(it->ref);
    ipt = parent.end();
  } else
    ipt = it->ref.get_sibling_iterator();
  insert(ipt, n);
  return it;
}
} // namespace semitree
