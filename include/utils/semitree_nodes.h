//------------------------------------------------------------------------------
//
// Definitions for semitree nodes.
//
// Nodes are separated into two categories.
// First is leaf nodes, second is branch nodes.
// Leaf nodes do not have any children.
// Contrary to them, branch nodes can have children.
//
// All nodes are node_t.
// Branch nodes are always represented by branch_t.
// Leaf nodes are always leaf_t
// To check whether node is branch, method is_branch provided.
// Methods for iteration are also available, check definitions of
// nodes to find all actual set.
//
// Nodes are paremetrized by payload: Leaf data for leaf nodes
// and Branch data for branch nodes.
//
// Nodes have protected constructors and destructors so they cannot
// be directly created by user. To use semitree, user should derive
// Leaf and Branch classes from leaf_t and branch_t correspondingly.
//
// Base class, node_t, inherits from special template class node_interface_t.
// This class can be specialized by user to extend node interface
// by delegating call to payload data. Additionally, node_interface_helper_t
// class provides delegate_call method to automatically choose between
// leaf and branch nodes and call corresponding methods. This is mainly needed
// in presence of custom rtti, but should be avoided if interface is extended
// with virtual functions.
//
// All setters for pointers are hidden and should be used only by corresponding
// tree_t manager class.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "semitree_iterator.h"

#include <cassert>

namespace semitree {

template <typename Leaf, typename Branch> class tree_t;
template <typename Leaf, typename Branch> class node_t;
template <typename Leaf, typename Branch> class branch_t;

template <typename Leaf, typename Branch> class node_interface_helper_t {
protected:
  using node_t = semitree::node_t<Leaf, Branch>;
  using branch_t_ = semitree::branch_t<Leaf, Branch>;

  template <typename T, typename F, typename... Args>
  static auto delegate_call(T &ths, F f, Args &&... args)
      -> decltype(f(ths, std::forward<Args>(args)...)) {
    using nt = std::conditional_t<std::is_const_v<T>, const node_t, node_t>;
    using lt = std::conditional_t<std::is_const_v<T>, const Leaf, Leaf>;
    using bt = std::conditional_t<std::is_const_v<T>, const Branch, Branch>;
    auto &node = static_cast<nt &>(ths);
    if (node.is_branch())
      return f(static_cast<bt &>(node), std::forward<Args>(args)...);
    return f(static_cast<lt &>(node), std::forward<Args>(args)...);
  }
};

// Class for extension of interface of tree node. Specialize this for your nodes
// to provide new functions that should be imported to node_t class.
// By default, no methods are added.
// Example:
// void dump(std::ostream &os) const {
//   auto fn = [](auto &n, std::ostream &os) { n.dump(os); };
//   forward_call(*this, fn, os);
// }
template <typename Leaf, typename Branch>
class node_interface_t : public node_interface_helper_t<Leaf, Branch> {};

// Node base class. Contains pointers to parent and left and right siblings.
template <typename Leaf, typename Branch>
class node_t : public node_interface_t<Leaf, Branch> {
  friend class tree_t<Leaf, Branch>;

  using branch_t = semitree::branch_t<Leaf, Branch>;

protected:
  // There are two fundamental types of nodes -- leaf and branch,
  // and one utilitary -- sentinel. Sentinel should not be accessed by
  // users. Sentinel entry here is pure for debugging reasons.
  enum class node_type_t { LEAF, BRANCH, SENTINEL };

private:
  branch_t *parent_;
  node_t *prev_;
  node_t *next_;
  node_type_t ntype_;

protected:
  node_t(node_type_t type)
      : parent_(nullptr), prev_(nullptr), next_(nullptr), ntype_(type) {}

public:
  ~node_t() = default;
  node_t(const node_t &) = delete;
  node_t &operator=(const node_t &) = delete;

  bool is_branch() const { return ntype_ == node_type_t::BRANCH; }
  node_t &get_prev() { return *prev_; }
  const node_t &get_prev() const { return *prev_; }
  node_t &get_next() { return *next_; }
  const node_t &get_next() const { return *next_; }
  bool has_parent() const { return parent_; }
  branch_t &get_parent() {
    assert(has_parent() && "Attempt to get parent of node without parent");
    return *parent_;
  }
  const branch_t &get_parent() const {
    assert(has_parent() && "Attempt to get parent of node without parent");
    return *parent_;
  }

  auto get_sibling_iterator() { return sibling_iterator_t(this); }

protected:
  void set_parent(branch_t *parent) { parent_ = parent; }
  void set_prev(node_t *prev) { prev_ = prev; }
  void set_next(node_t *next) { next_ = next; }
};

// Leaf node. Convenience class that should be used as a base of Leaf.
template <typename Leaf, typename Branch>
class leaf_t : public node_t<Leaf, Branch> {
  friend class tree_t<Leaf, Branch>;

  using node_t = semitree::node_t<Leaf, Branch>;
  using node_type_t = typename node_t::node_type_t;

protected:
  leaf_t() : node_t(node_type_t::LEAF) {}

public:
  ~leaf_t() = default;

private:
  // Hide these from derived classes.
  using node_t::set_next;
  using node_t::set_parent;
  using node_t::set_prev;
};

// Parent node. Contains special sentinel node.
// Sentinel node contains pointers to first and last children
// of parent node. Sentinel node itself denote end child iterator.
// All above means that first child is next node of sentinel and
// last child is previous node of sentinel.
template <typename Leaf, typename Branch>
class branch_t : public node_t<Leaf, Branch> {
  friend class tree_t<Leaf, Branch>;

  using node_t = semitree::node_t<Leaf, Branch>;
  using node_type_t = typename node_t::node_type_t;

  // Sentinel node denoting end of children list.
  // On construction has parent and points always to itself.
  struct sentinel_t : public node_t {
    sentinel_t(branch_t *parent) : node_t(node_type_t::SENTINEL) {
      node_t::set_parent(parent);
      node_t::set_prev(this);
      node_t::set_next(this);
    }

    using node_t::set_next;
    using node_t::set_prev;
  };

private:
  sentinel_t sent_;

  // Debug check for validity.
  void check_branch() const {
    assert(node_t::is_branch() &&
           "Attempt to call branch methods on invalid node type");
  }

protected:
  branch_t() : node_t(node_type_t::BRANCH), sent_(this) {}

public:
  ~branch_t() = default;

  // TODO: add insertion methods.

  node_t &get_firstchild() {
    check_branch();
    return sent_.get_next();
  }
  const node_t &get_firstchild() const {
    check_branch();
    return sent_.get_next();
  }
  node_t &get_lastchild() {
    check_branch();
    return sent_.get_prev();
  }
  const node_t &get_lastchild() const {
    check_branch();
    return sent_.get_prev();
  }

  bool empty() const {
    check_branch();
    return &sent_.get_next() == &sent_;
  }

  auto begin() { return sibling_iterator_t(&get_firstchild()); }
  auto end() { return sibling_iterator_t(&sent_); }

  // TODO: implement const iterators.
  auto begin() const { return const_cast<branch_t *>(this)->begin(); }
  auto end() const { return const_cast<branch_t *>(this)->end(); }

private:
  // Hide these from derived classes.
  using node_t::set_next;
  using node_t::set_parent;
  using node_t::set_prev;

  void set_firstchild(node_t *fc) {
    check_branch();
    sent_.set_next(fc);
  }
  void set_lastchild(node_t *lc) {
    check_branch();
    sent_.set_prev(lc);
  }
};

} // namespace semitree
