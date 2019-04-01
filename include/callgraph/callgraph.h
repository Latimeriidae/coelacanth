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

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace tg {
class typegraph_t;
}

namespace cg {

enum class calltype_t : unsigned char { DIRECT, CONDITIONAL, INDIRECT };

struct vertexprop_t {
  int funcid, componentno, depth, nparms;
  std::string get_name() const;
};

struct edgeprop_t {
  calltype_t calltype;
};

// type for callgraph
using cgraph_t =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                          vertexprop_t, edgeprop_t>;
using vertex_iter_t = boost::graph_traits<cgraph_t>::vertex_iterator;
using edge_iter_t = boost::graph_traits<cgraph_t>::edge_iterator;
using vertex_t = boost::graph_traits<cgraph_t>::vertex_descriptor;

class callgraph_t {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  cgraph_t graph_;
  std::set<vertex_t> non_leafs_;
  std::set<vertex_t> leafs_;

public:
  callgraph_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>);

  // following interface to implement:
  // begin/end for function iterator
  // number of functions
  // function by number
  // callees or function with call type
  // callers or function with call type
  void dump(std::ostream &os);
};

} // namespace cg
