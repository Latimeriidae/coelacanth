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

#include "calliters.h"
#include "calltypes.h"
#include "config/configs.h"
#include "funcmeta.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace tg {
class typegraph_t;
}

namespace cg {

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

  vertex_iter_t begin() const;
  vertex_iter_t end() const;

  callee_iterator_t callees_begin(vertex_t v, calltype_t mask) const;
  callee_iterator_t callees_end(vertex_t v, calltype_t mask) const;
  caller_iterator_t callers_begin(vertex_t v, calltype_t mask) const;
  caller_iterator_t callers_end(vertex_t v, calltype_t mask) const;

  vertexprop_t vertex_from(vertex_t v) const { return graph_[v]; }
  vertex_t dest_from(edge_t e) const { return boost::target(e, graph_); }
  vertex_t src_from(edge_t e) const { return boost::source(e, graph_); }

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
  std::pair<int, std::vector<int>> gen_params(vertex_t v);
  int pick_typeid(vertex_t v, bool allow_void = false, bool ret_type = false);
  void map_modules();
};

} // namespace cg
