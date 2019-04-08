//------------------------------------------------------------------------------
//
//  Type categories support
//
//  This file defines all type categories and graph type for typegraph
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace tg {

enum class category_t : unsigned char {
  SCALAR = 0,
  STRUCT,
  ARRAY,
  POINTER,
  CATMAX
};

struct scalar_desc_t {
  std::string name;
  int size;
  bool is_float;
  bool is_signed;

  scalar_desc_t(const char *nm, int sz, bool isf, bool iss)
      : name(nm), size(sz), is_float(isf), is_signed(iss) {}
};

struct scalar_t {
  const scalar_desc_t *sdesc;
  explicit scalar_t(const scalar_desc_t *desc = nullptr) : sdesc(desc) {}
};

struct struct_t {
  // first pair element is child id
  // second is bitfield size
  std::vector<std::pair<int, int>> bitfields_;
};

struct array_t {
  int nitems;
};

struct pointer_t {};

using common_t = std::variant<scalar_t, struct_t, array_t, pointer_t>;

struct vertexprop_t {
  int id;
  category_t cat;
  common_t type;
  std::string get_short_name() const;
  std::string get_name() const;
};

std::ostream &operator<<(std::ostream &os, vertexprop_t v);

using tgraph_t = boost::adjacency_list<boost::vecS, boost::vecS,
                                       boost::bidirectionalS, vertexprop_t>;
using vertex_iter_t = boost::graph_traits<tgraph_t>::vertex_iterator;
using edge_iter_t = boost::graph_traits<tgraph_t>::edge_iterator;
using outedge_iter_t = boost::graph_traits<tgraph_t>::out_edge_iterator;
using vertex_t = boost::graph_traits<tgraph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<tgraph_t>::edge_descriptor;

} // namespace tg