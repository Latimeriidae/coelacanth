//------------------------------------------------------------------------------
//
// High-level abstract interface for call-graph
//
// Random call graph consists (obviously) from vertices and edges
// vertex properties: funcid, componentno, depth, nparms
// edge properties: calltype
//
// From users' perspective each function have its callee and caller sets
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "config/configs.h"
#include "typegraph/typegraph.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace cg {

enum class calltype_t : unsigned char { DIRECT, CONDITIONAL, INDIRECT };

struct vertexprop_t {
  int funcid, componentno, depth, nparms;
};

struct edgeprop_t {
  calltype_t calltype;
};

// type for callgraph
using cgraph_t =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                          vertexprop_t, edgeprop_t>;
using vertex_iter_t = boost::graph_traits<cgraph_t>::vertex_iterator;
using edge_iter_t = boost::graph_traits<cgraph_t>::edge_iterator;

class callgraph_t {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  cgraph_t graph_;

public:
  callgraph_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>);

  // following interface to implement:
  // begin/end for function iterator
  // dump
  // number of functions
  // function by number
  // callees or function with call type
  // callers or function with call type
};

} // namespace cg
