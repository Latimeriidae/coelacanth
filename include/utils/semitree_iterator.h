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

#include <iterator>
#include <utility>

namespace semitree {

// Avoid cyclic dependency on nodes.
template <typename Leaf, typename Branch> class node_t;
template <typename Leaf, typename Branch> class branch_t;
template <typename Leaf, typename Branch> class inorder_iterator_t;

// Simple bidirectional node iterator.
template <typename Leaf, typename Branch, bool IsConst>
class sibling_iterator_base_t final {
  using node_t = semitree::node_t<Leaf, Branch>;

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
template <typename Leaf, typename Branch> class inorder_iterator_t final {
  using node_t = semitree::node_t<Leaf, Branch>;
  using branch_t = semitree::branch_t<Leaf, Branch>;

  // Internal value that iterator contains.
  // Consists of pointer to node and visited field.
  struct internal_value_t final {
    node_t *ptr_;
    bool visited_;

    internal_value_t(node_t *ptr, bool visited)
        : ptr_{ptr}, visited_{visited} {}
  };

  // Reference type that user will see calling operator*.
  // Contains reference to node and visited boolean.
  struct internal_ref_t final {
    node_t &ref;
    bool visited;

    internal_ref_t(node_t &r, bool v) : ref{r}, visited{v} {}
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
    pointer_value_t(value_type &&ref) : ref_{std::move(ref)} {}
    value_type *operator->() { return &ref_; }
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
  inorder_iterator_t() : val_{nullptr, false} {}
  inorder_iterator_t(node_t *ptr, bool visited) : val_{ptr, visited} {}

  reference operator*() const { return reference{*val_.ptr_, val_.visited_}; }
  pointer operator->() const { return pointer_value_t{**this}; }

  inorder_iterator_t &operator++();
  inorder_iterator_t operator++(int) {
    auto it{*this};
    ++(*this);
    return it;
  }

  inorder_iterator_t &operator--();
  inorder_iterator_t operator--(int) {
    auto it{*this};
    --(*this);
    return it;
  }

  bool equals(inorder_iterator_t rhs) const {
    // Comparison of simple nodes. Just look at the pointers.
    if (!val_.ptr_->is_branch() && !rhs.val_.ptr_->is_branch()) {
      return val_.ptr_ == rhs.val_.ptr_;
    }

    if (val_.ptr_->is_branch()) {
      // Parent node and parent node.
      // They are equal only if all internal state is equal.
      if (rhs.val_.ptr_->is_branch())
        return val_.ptr_ == rhs.val_.ptr_ && val_.visited_ == rhs.val_.visited_;
      // Parent node and simple node are always not equal.
      return false;
    }
    // The same as in previous comment.
    return false;
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
template <typename Leaf, typename Branch>
auto inorder_iterator_t<Leaf, Branch>::operator++() -> inorder_iterator_t & {
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
template <typename Leaf, typename Branch>
auto inorder_iterator_t<Leaf, Branch>::operator--() -> inorder_iterator_t & {
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
inline bool operator==(inorder_iterator_t<Leaf, Branch> lhs,
                       inorder_iterator_t<Leaf, Branch> rhs) {
  return lhs.equals(rhs);
}

template <typename Leaf, typename Branch>
inline bool operator!=(inorder_iterator_t<Leaf, Branch> lhs,
                       inorder_iterator_t<Leaf, Branch> rhs) {
  return !(lhs == rhs);
}
} // namespace semitree
