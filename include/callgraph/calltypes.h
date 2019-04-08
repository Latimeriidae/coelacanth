//------------------------------------------------------------------------------
//
//  Function types support
//
//  This file defines all function types and graph type for callgraph
//
//  We have no distinguished INDIRECT call type because call graph have no
//  indirect edges
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "config/configs.h"
#include "funcmeta.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace tg {
class typegraph_t;
}

namespace cg {

enum class calltype_t : unsigned char { DIRECT = 1, CONDITIONAL = 2 };

struct vertexprop_t {
  int funcid = -1;
  int componentno = -1;
  int indset = 0;
  int rettype = -1;
  ms::metanode_t metainfo;
  std::vector<int> argtypes;
  std::string get_name(std::shared_ptr<tg::typegraph_t>) const;
  std::string get_color() const;
};

struct edgeprop_t {
  calltype_t calltype = calltype_t::CONDITIONAL;
  std::string get_style() const;
  std::string get_color() const;
};

// type for callgraph
using cgraph_t =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                          vertexprop_t, edgeprop_t>;
using vertex_iter_t = boost::graph_traits<cgraph_t>::vertex_iterator;
using edge_iter_t = boost::graph_traits<cgraph_t>::edge_iterator;
using outedge_iter_t = boost::graph_traits<cgraph_t>::out_edge_iterator;
using inedge_iter_t = boost::graph_traits<cgraph_t>::in_edge_iterator;
using vertex_t = boost::graph_traits<cgraph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<cgraph_t>::edge_descriptor;

} // namespace cg