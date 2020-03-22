//------------------------------------------------------------------------------
//
// Definitions for semitree iterators.
//
// There are two basic types of iterators in semitree:
// inorder iterators and sibling iterators.
//
// Inorder iterators allow to traverse semitree in order.
// As example, the program should be printed in this order
// like we are walking AST and dump whatever nodes contain.
// Inorder iterators have special field ''visited'' to recognize
// whether internal node was visited before or not since inorder
// traversal will visit such nodes twice -- once when we first encountered
// node and second when we are returning from last child of this node.
//
// Sibling iterators are more conventional. These are simple double-linked list
// iterators that allow to go between nodes on the same level.
//
// Every inorder iterator can be converted in sibling iterator and vice versa.
//
// TODO: convesion of sibling iterators to inorder.
// TODO: const iterators.
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>
#include <utility>

namespace semitree {

// Avoid cyclic dependency on nodes.
template <typename Leaf, typename Branch> class node_t;
template <typename Leaf, typename Branch> class branch_t;
template <typename Leaf, typename Branch, bool IsConst>
class inorder_iterator_base_t;

// Simple bidirectional node iterator.
template <typename Leaf, typename Branch, bool IsConst>
class sibling_iterator_base_t final {
  using node_t = semitree::node_t<Leaf, Branch>;
  using branch_t = semitree::branch_t<Leaf, Branch>;
  using inorder_iterator_base_t =
      semitree::inorder_iterator_base_t<Leaf, Branch, IsConst>;

public:
  using difference_type = std::ptrdiff_t;
  using value_type = std::conditional_t<IsConst, const node_t, node_t>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::bidirectional_iterator_tag;

private:
  pointer ptr_ = nullptr;

public:
  sibling_iterator_base_t() = default;
  explicit sibling_iterator_base_t(pointer ptr) noexcept : ptr_{ptr} {}

  // Ctor from inorder iterator. If 'it' points to visited branch then
  // new iterator will point to 'it->end()'. Otherwise, just copy pointer.
  sibling_iterator_base_t(inorder_iterator_base_t it) noexcept
      : ptr_{it.is_null() ? nullptr : &it->ref} {
    if (!ptr_)
      return;
    if (ptr_->is_branch() && it->visited) {
      using br_t = std::conditional_t<IsConst, const branch_t *, branch_t *>;
      ptr_ = static_cast<br_t>(ptr_)->end().ptr_;
    }
  }

  sibling_iterator_base_t(const sibling_iterator_base_t &) = default;
  sibling_iterator_base_t &operator=(const sibling_iterator_base_t &) = default;

  // Ctor for const_iterator from iterator.
  template <bool IsConst_ = IsConst, typename = std::enable_if_t<IsConst_>>
  sibling_iterator_base_t(
      sibling_iterator_base_t<Leaf, Branch, !IsConst_> rhs) noexcept
      : ptr_{rhs.operator->()} {}

  reference operator*() const noexcept { return *ptr_; }
  pointer operator->() const noexcept { return ptr_; }

  sibling_iterator_base_t &operator++() noexcept {
    ptr_ = &ptr_->get_next();
    return *this;
  }
  sibling_iterator_base_t operator++(int) noexcept {
    auto it{*this};
    ++(*this);
    return it;
  }

  sibling_iterator_base_t &operator--() noexcept {
    ptr_ = &ptr_->get_prev();
    return *this;
  }
  sibling_iterator_base_t operator--(int) noexcept {
    auto it{*this};
    --(*this);
    return it;
  }

  friend bool operator==(sibling_iterator_base_t lhs,
                         sibling_iterator_base_t rhs) noexcept {
    return lhs.ptr_ == rhs.ptr_;
  }

  friend bool operator!=(sibling_iterator_base_t lhs,
                         sibling_iterator_base_t rhs) noexcept {
    return lhs.ptr_ != rhs.ptr_;
  }
};

template <typename Leaf, typename Branch>
using sibling_iterator_t = sibling_iterator_base_t<Leaf, Branch, false>;

template <typename Leaf, typename Branch>
using const_sibling_iterator_t = sibling_iterator_base_t<Leaf, Branch, true>;

// Bidirectional inorder iterator. Its structure is more complicated than
// sibling iterator one. In addition to pointer to node, it contains
// boolean field indicating whether node was visited or not.
// Equals method is more complicated than just comparison of pointer and
// and visited field since operator++ and operator-- can give different
// internal states when for the node.
// Presense of visited field forces iterator to be more complex structure.
// To support visited field there are some internal helper classes used
// as proxy classes, for example, ''pointer_value''.
template <typename Leaf, typename Branch, bool IsConst>
class inorder_iterator_base_t final {
  using node_t = semitree::node_t<Leaf, Branch>;
  using branch_t = semitree::branch_t<Leaf, Branch>;
  using sibling_iterator_base_t =
      semitree::sibling_iterator_base_t<Leaf, Branch, IsConst>;

  using internal_value_type = std::conditional_t<IsConst, const node_t, node_t>;
  using internal_pointer = internal_value_type *;
  using internal_reference = internal_value_type &;

  // Internal value that iterator contains.
  // Consists of pointer to node and visited field.
  struct internal_value_t final {
    internal_pointer ptr_ = nullptr;
    bool visited_ = false;

    internal_value_t() = default;
    internal_value_t(internal_pointer ptr, bool visited) noexcept
        : ptr_{ptr}, visited_{visited} {}
  };

  // Reference type that user will see calling operator*.
  // Contains reference to node and visited boolean.
  struct internal_ref_t final {
    internal_reference ref;
    bool visited;

    internal_ref_t(internal_reference r, bool v) noexcept
        : ref{r}, visited{v} {}
  };

  // Proxy class to implement complex pointer to
  // reference type. This proxy prolongates lifetime of
  // reference so user can write it->something
  // because in this case there will be temporary pointer_value
  // that will hold reference to actual node.
  class pointer_value_t final {
    using value_type = internal_ref_t;
    value_type ref_;

  public:
    pointer_value_t(value_type &&ref) noexcept : ref_{std::move(ref)} {}
    value_type *operator->() noexcept { return &ref_; }
  };

public:
  using difference_type = std::ptrdiff_t;
  using value_type = internal_ref_t;
  using pointer = pointer_value_t;
  using reference = internal_ref_t;
  using iterator_category = std::bidirectional_iterator_tag;

private:
  internal_value_t val_;

public:
  inorder_iterator_base_t() = default;
  inorder_iterator_base_t(internal_pointer ptr, bool visited) noexcept
      : val_{ptr, visited} {}

  // Ctor from sibling iterator and visited state.
  // If 'it' points to leaf then visited state is ignored.
  // If 'it' is past-the-end iterator then new inorder iterator
  // points to visited parent and 'visited' flag is ignored.
  // In this case insert will behave identical when it is called either with
  // 'it' or with created inorder iterator.
  inorder_iterator_base_t(sibling_iterator_base_t it, bool visited) noexcept
      : val_{it.operator->(), visited} {
    if (is_null())
      return;
    assert(it->has_parent() && "Attempt to create iterator from orphan node");
    if (it->get_parent().end() != it)
      return;
    val_.ptr_ = &it->get_parent();
    val_.visited_ = true;
  }

  inorder_iterator_base_t(const inorder_iterator_base_t &) = default;
  inorder_iterator_base_t &operator=(const inorder_iterator_base_t &) = default;

  // Ctor for constant iterator from non-constant.
  template <bool IsConst_ = IsConst, typename = std::enable_if_t<IsConst_>>
  inorder_iterator_base_t(
      inorder_iterator_base_t<Leaf, Branch, !IsConst_> rhs) noexcept
      : val_{(rhs.is_null() ? nullptr : &rhs->ref), rhs->visited} {}

  bool is_null() const noexcept { return !val_.ptr_; }

  reference operator*() const noexcept {
    return reference{*val_.ptr_, val_.visited_};
  }
  pointer operator->() const noexcept { return pointer{**this}; }

  inorder_iterator_base_t &operator++() noexcept;
  inorder_iterator_base_t operator++(int) noexcept {
    auto it{*this};
    ++(*this);
    return it;
  }

  inorder_iterator_base_t &operator--() noexcept;
  inorder_iterator_base_t operator--(int) noexcept {
    auto it{*this};
    --(*this);
    return it;
  }

  friend bool operator!=(inorder_iterator_base_t lhs,
                         inorder_iterator_base_t rhs) noexcept {
    // If pointers differ then iterators differ.
    if (lhs.val_.ptr_ != rhs.val_.ptr_)
      return true;

    // There can be singular iterators.
    if (!lhs.val_.ptr_)
      return false;

    const bool lbr = lhs.val_.ptr_->is_branch();
    const bool rbr = rhs.val_.ptr_->is_branch();
    // If both lhs and rhs are branches or leafs, then leafs are equal,
    // and branches are not equal only if visited state differs.
    if (lbr == rbr)
      return lbr && lhs.val_.visited_ != rhs.val_.visited_;
    // All other combinations are not equal.
    return true;
  }

  friend bool operator==(inorder_iterator_base_t lhs,
                         inorder_iterator_base_t rhs) noexcept {
    return !(lhs != rhs);
  }
};

// Get next node inorder.
// Logic is quite complicated.
// For parent we need to step in to first child if parent
// is not visited or set parent to be visited if it is empty.
// If parent is visited then it is treated as simple node.
// For simple node we try move forward as in sibling iterator.
// If this iterator pointed to lastchild of some parent
// then we need to move to this parent.
template <typename Leaf, typename Branch, bool IsConst>
auto inorder_iterator_base_t<Leaf, Branch, IsConst>::operator++() noexcept
    -> inorder_iterator_base_t & {
  // Unvisited parent. Go to unvisited firstchild or to
  // visited itself if it is empty parent.
  if (val_.ptr_->is_branch() && !val_.visited_) {
    auto *parent = static_cast<branch_t *>(val_.ptr_);
    if (parent->empty())
      val_.visited_ = true;
    else
      val_.ptr_ = &parent->get_firstchild();
    return *this;
  }

  // Else go forward.
  // First case if for non-function nodes.
  if (val_.ptr_->has_parent()) {
    branch_t &parent = val_.ptr_->get_parent();
    // If node is last child then go to parent and set it to
    // be visited.
    if (&parent.get_lastchild() == val_.ptr_) {
      val_.ptr_ = &parent;
      val_.visited_ = true;
    } else {
      val_.ptr_ = &val_.ptr_->get_next();
      val_.visited_ = false;
    }
  } else {
    // If it is a function then there is no need to check parent.
    val_.ptr_ = &val_.ptr_->get_next();
    val_.visited_ = false;
  }
  return *this;
}

// Do the same as in ++ but in reverse. Interesting fact is that
// after ++ and -- we can get different internal states.
// This should be handled in equals method.
template <typename Leaf, typename Branch, bool IsConst>
auto inorder_iterator_base_t<Leaf, Branch, IsConst>::operator--() noexcept
    -> inorder_iterator_base_t & {
  // Parent is visited. Next is either unvisited parent itself
  // if it is empty, or visited lastchild in other case.
  if (val_.ptr_->is_branch() && val_.visited_) {
    auto *parent = static_cast<branch_t *>(val_.ptr_);
    if (parent->empty())
      val_.visited_ = false;
    else
      val_.ptr_ = &parent->get_lastchild();
    return *this;
  }

  if (val_.ptr_->has_parent()) {
    branch_t &parent = val_.ptr_->get_parent();
    // If node is firstchild then go upward and set
    // parent to be unvisited.
    if (val_.ptr_ == &parent.get_firstchild()) {
      val_.ptr_ = &parent;
      val_.visited_ = false;
    } else {
      val_.ptr_ = &val_.ptr_->get_prev();
      val_.visited_ = true;
    }
  } else {
    val_.ptr_ = &val_.ptr_->get_prev();
    val_.visited_ = true;
  }

  return *this;
}

template <typename Leaf, typename Branch>
using inorder_iterator_t = inorder_iterator_base_t<Leaf, Branch, false>;

template <typename Leaf, typename Branch>
using const_inorder_iterator_t = inorder_iterator_base_t<Leaf, Branch, true>;
} // namespace semitree
