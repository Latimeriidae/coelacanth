//------------------------------------------------------------------------------
//
// High-level type-graph value-type
//
// type-graph consists of types:
// scalar types (integer, fp, etc)
// array types
// structure types
// pointers
//
// from user perspective most often query to typegraph is query of random type
// with certain limitations
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "config/configs.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace tg {

enum class type_category_t : unsigned char { SCALAR, STRUCT, ARRAY, POINTER };

struct vertexprop_t {
  int id;
  type_category_t cat;
  bool is_float;
  bool is_signed;
};

// type for callgraph
using tgraph_t = boost::adjacency_list<boost::vecS, boost::vecS,
                                       boost::directedS, vertexprop_t>;
using vertex_iter_t = boost::graph_traits<tgraph_t>::vertex_iterator;
using edge_iter_t = boost::graph_traits<tgraph_t>::edge_iterator;

class typegraph_t {
  cfg::config config_;
  tgraph_t graph_;

public:
  typegraph_t(cfg::config &&);

  // TODO: interface
};

} // namespace tg
