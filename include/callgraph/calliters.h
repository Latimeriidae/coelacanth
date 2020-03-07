//------------------------------------------------------------------------------
//
//  Function iterators support
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <utility>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include "calltypes.h"

namespace cg {

class callgraph_t;

vertexprop_t vertex_from(const callgraph_t *, vertex_t);
vertex_t dest_from(const callgraph_t *, edge_t);
vertex_t src_from(const callgraph_t *, edge_t);

// neighbour iterator traversing neighbours (childs/parents) of given vertex
template <typename eit> class nbr_iterator_t {
  const callgraph_t *cgp_;
  eit ei_;
  calltype_t mask_;

public:
  nbr_iterator_t(const callgraph_t *cgp, eit ei, calltype_t mask)
      : cgp_(cgp), ei_(ei), mask_(mask) {
    // TODO: we shall here skip from eit all that is not conform to mask
    // say function have outedges: c c i c d d c i
    // then on indirect mask we shall skip first 2 cs
  }

  nbr_iterator_t(const nbr_iterator_t &) = default;
  nbr_iterator_t &operator=(const nbr_iterator_t &) = default;

  nbr_iterator_t(nbr_iterator_t &&) = default;
  nbr_iterator_t &operator=(nbr_iterator_t &&) = default;

  using iterator_type = nbr_iterator_t;

  // never random access because it may skip some functions under the mask
  using iterator_category = std::bidirectional_iterator_tag;

  using value_type = vertex_t;
  using difference_type = int;
  using pointer = value_type *;
  using reference = value_type &;

  nbr_iterator_t &operator++() {
    // TODO: skip along the way
    ++ei_;
    return *this;
  }

  nbr_iterator_t operator++(int) {
    auto temp(*this);
    ++(*this);
    return temp;
  }

  nbr_iterator_t &operator+=(int n) {
    for (int i = 0; i < n; ++i)
      ++(*this);
    return *this;
  }

  nbr_iterator_t &operator--() {
    // TODO: skip along the way
    --ei_;
    return *this;
  }

  nbr_iterator_t operator--(int) {
    auto temp(*this);
    --(*this);
    return temp;
  }

  nbr_iterator_t &operator-=(int n) {
    for (int i = 0; i < n; ++i)
      --(*this);
    return *this;
  }

  value_type operator*() const;

  bool equals(const nbr_iterator_t &lhs) const {
    return (lhs.cgp_ == cgp_) && (lhs.ei_ == ei_);
  }

  eit base() { return ei_; }
};

template <>
nbr_iterator_t<outedge_iter_t>::value_type
    nbr_iterator_t<outedge_iter_t>::operator*() const;

template <>
nbr_iterator_t<inedge_iter_t>::value_type
    nbr_iterator_t<inedge_iter_t>::operator*() const;

using callee_iterator_t = nbr_iterator_t<outedge_iter_t>;
using caller_iterator_t = nbr_iterator_t<inedge_iter_t>;

static inline bool operator==(callee_iterator_t lhs, callee_iterator_t rhs) {
  return lhs.equals(rhs);
}

static inline bool operator!=(callee_iterator_t lhs, callee_iterator_t rhs) {
  return !lhs.equals(rhs);
}

static inline callee_iterator_t operator+(callee_iterator_t it, int n) {
  it += n;
  return it;
}

static inline callee_iterator_t operator-(callee_iterator_t it, int n) {
  it -= n;
  return it;
}

static inline bool operator==(caller_iterator_t lhs, caller_iterator_t rhs) {
  return lhs.equals(rhs);
}

static inline bool operator!=(caller_iterator_t lhs, caller_iterator_t rhs) {
  return !lhs.equals(rhs);
}

static inline caller_iterator_t operator+(caller_iterator_t it, int n) {
  it += n;
  return it;
}

static inline caller_iterator_t operator-(caller_iterator_t it, int n) {
  it -= n;
  return it;
}

} // namespace cg