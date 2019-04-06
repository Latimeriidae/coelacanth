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
  int funcid = -1, componentno = -1, indset = 0;
  int rettype = -1;
  unsigned usesigned : 1;
  unsigned usefloat : 1;
  unsigned usecomplex : 1;
  unsigned usepointers : 1;
  std::vector<int> argtypes;
  std::string get_name() const;
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
using vertex_t = boost::graph_traits<cgraph_t>::vertex_descriptor;
using edge_t = boost::graph_traits<cgraph_t>::edge_descriptor;

class callgraph_t final {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  cgraph_t graph_;

  // construction support structures
private:
  std::set<vertex_t> non_leafs_;
  std::set<vertex_t> leafs_;
  std::vector<std::vector<vertex_t>> comps_;
  std::vector<vertex_t> inds_;

  // public interface
public:
  explicit callgraph_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>);

  // following interface to implement:

  // begin/end for descriptor iterator
  // begin/end for function iterator
  // function by descriptor
  // callees of function (by call type)
  // callers of function (by call type)
  void dump(std::ostream &os) const;

  // construction helpers
private:
  void generate_random_graph(int nvertices);
  void process_leafs();
  void connect_components();
  void add_self_loops();
  void create_indcalls();
  void decide_metastructure();
  void assign_types();
  void map_modules();
};

} // namespace cg
