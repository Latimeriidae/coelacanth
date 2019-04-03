//------------------------------------------------------------------------------
//
//  Type iterators support
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

#include "typecats.h"

namespace tg {

class typegraph_t;

vertexprop_t vertex_from(const typegraph_t *, vertex_t);
vertex_t dest_from(const typegraph_t *, edge_t);

// common type iterator traversing whole typegraph
class ct_iterator_t {
  const typegraph_t *tgp_;
  vertex_iter_t vi_;

public:
  ct_iterator_t(const typegraph_t *tgp, vertex_iter_t vi)
      : tgp_(tgp), vi_(vi) {}

  using iterator_type = ct_iterator_t;
  using iterator_category = typename vertex_iter_t::iterator_category;
  using value_type = std::pair<vertex_t, common_t>;

  ct_iterator_t &operator++() {
    ++vi_;
    return *this;
  }

  ct_iterator_t operator++(int) {
    auto temp(*this);
    ++vi_;
    return temp;
  }

  ct_iterator_t &operator+=(int n) {
    vi_ += n;
    return *this;
  }

  ct_iterator_t &operator--() {
    --vi_;
    return *this;
  }

  ct_iterator_t operator--(int) {
    auto temp(*this);
    --vi_;
    return temp;
  }

  ct_iterator_t &operator-=(int n) {
    vi_ -= n;
    return *this;
  }

  value_type operator*() const {
    auto v = *vi_;
    auto vtx = vertex_from(tgp_, v);
    return std::make_pair(v, vtx.type);
  }

  bool equals(const ct_iterator_t &lhs) const {
    return (lhs.tgp_ == tgp_) && (lhs.vi_ == vi_);
  }

  vertex_iter_t base() { return vi_; }
};

static inline bool operator==(ct_iterator_t lhs, ct_iterator_t rhs) {
  return lhs.equals(rhs);
}

static inline bool operator!=(ct_iterator_t lhs, ct_iterator_t rhs) {
  return !lhs.equals(rhs);
}

static inline ct_iterator_t operator+(ct_iterator_t it, int n) {
  it += n;
  return it;
}

static inline ct_iterator_t operator-(ct_iterator_t it, int n) {
  it -= n;
  return it;
}

// child iterator traversing childs of given vertex
class child_iterator_t {
  const typegraph_t *tgp_;
  outedge_iter_t ei_;

public:
  child_iterator_t(const typegraph_t *tgp, outedge_iter_t ei)
      : tgp_(tgp), ei_(ei) {}
  using iterator_type = child_iterator_t;
  using iterator_category = typename outedge_iter_t::iterator_category;
  using value_type = std::pair<vertex_t, common_t>;

  child_iterator_t &operator++() {
    ++ei_;
    return *this;
  }

  child_iterator_t operator++(int) {
    auto temp(*this);
    ++ei_;
    return temp;
  }

  child_iterator_t &operator+=(int n) {
    ei_ += n;
    return *this;
  }

  child_iterator_t &operator--() {
    --ei_;
    return *this;
  }

  child_iterator_t operator--(int) {
    auto temp(*this);
    --ei_;
    return temp;
  }

  child_iterator_t &operator-=(int n) {
    ei_ -= n;
    return *this;
  }

  value_type operator*() const {
    auto v = dest_from(tgp_, *ei_);
    auto vtx = vertex_from(tgp_, v);
    return std::make_pair(v, vtx.type);
  }

  bool equals(const child_iterator_t &lhs) const {
    return (lhs.tgp_ == tgp_) && (lhs.ei_ == ei_);
  }

  outedge_iter_t base() { return ei_; }
};

static inline bool operator==(child_iterator_t lhs, child_iterator_t rhs) {
  return lhs.equals(rhs);
}

static inline bool operator!=(child_iterator_t lhs, child_iterator_t rhs) {
  return !lhs.equals(rhs);
}

static inline child_iterator_t operator+(child_iterator_t it, int n) {
  it += n;
  return it;
}

static inline child_iterator_t operator-(child_iterator_t it, int n) {
  it -= n;
  return it;
}

} // namespace tg