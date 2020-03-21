//------------------------------------------------------------------------------
//
// Definition of sample testing semitree.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "utils/semitree.h"

class leaf;
class branch;

namespace semitree {
template <>
class node_interface_t<leaf, branch> : node_interface_helper_t<leaf, branch> {
public:
  int get_data() const {
    auto fn = [](auto &n) { return n.get_data(); };
    return delegate_call(*this, fn);
  }
};
} // namespace semitree

using node = semitree::node_t<leaf, branch>;
using sib_it = semitree::sibling_iterator_t<leaf, branch>;
using ino_it = semitree::inorder_iterator_t<leaf, branch>;

class leaf : public semitree::leaf_t<leaf, branch> {
  int data_ = 0;

public:
  leaf() = default;
  explicit leaf(int d) : data_{d} {}

  int get_data() const { return data_; }
};
class branch : public semitree::branch_t<leaf, branch> {
  int data_ = 0;

public:
  branch() = default;
  explicit branch(int d) : data_{d} {}

  int get_data() const { return data_; }
};

class tree : public semitree::tree_t<leaf, branch> {};

inline std::ostream &boost_test_print_type(std::ostream &os, sib_it it) {
  return os << "sib_it(" << &*it << ")";
}

inline std::ostream &boost_test_print_type(std::ostream &os, ino_it it) {
  auto val{*it};
  return os << "ino_it(" << &val.ref
            << (val.visited ? ",visited" : ",not visited") << ")";
}
