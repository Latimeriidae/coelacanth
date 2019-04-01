//------------------------------------------------------------------------------
//
// Callgraph impl
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>

#include <boost/graph/graphviz.hpp>
#include <boost/graph/random.hpp>

#include "callgraph.h"
#include "typegraph/typegraph.h"

namespace cg {

// label for dot dump of callgraph
std::string vertexprop_t::get_name() const {
  std::ostringstream s;
  s << "f";
  return s.str();
}

//------------------------------------------------------------------------------
//
// Callgraph
//
//------------------------------------------------------------------------------

struct config_rng {
  using result_type = int;
  cfg::config &cf_;
  config_rng(cfg::config &cf) : cf_(cf) {}
  result_type operator()() { return cf_.rand_positive(); }
  int min() { return 0; }
  int max() { return std::numeric_limits<int>::max(); }
};

callgraph_t::callgraph_t(cfg::config &&cf,
                         std::shared_ptr<tg::typegraph_t> tgraph)
    : config_(std::move(cf)), tgraph_(tgraph) {
  if (!config_.quiet())
    std::cout << "Creating callgraph" << std::endl;

  // 1. random graph
  config_rng cfrng(config_);
  int nvertices = cfg::get(config_, CG::VERTICES);
  int nedges = cfg::get(config_, CG::EDGES);
  boost::generate_random_graph(graph_, nvertices, nedges, cfrng,
                               /* par */ false, /* self */ true);

  // 2. adding more leafs
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    if (boost::degree(*vi, graph_) == 0)
      leafs_.insert(*vi);
    else
      non_leafs_.insert(*vi);

  int naddleafs = cfg::get(config_, CG::ADDLEAFS);
  for (int i = 0; i < naddleafs; ++i) {
    auto it = non_leafs_.begin();
    int n = config_.rand_positive() % non_leafs_.size();
    std::advance(it, n);

    auto sv = boost::add_vertex(graph_);
    leafs_.insert(sv);
    boost::add_edge(*it, sv, graph_);
  }

  // 3. spanning trees
  // 4. component roots
  // --- here graph structure is frozen ---
  // 5. function and return types
}

// dump as dot file
void callgraph_t::dump(std::ostream &os) {
  boost::dynamic_properties dp;
  auto bundle = boost::get(boost::vertex_bundle, graph_);
  dp.property("node_id", boost::get(boost::vertex_index, graph_));
  dp.property("label", boost::make_transform_value_property_map(
                           std::mem_fn(&vertexprop_t::get_name), bundle));
  boost::write_graphviz_dp(os, graph_, dp);
}

} // namespace cg

// task system support
std::shared_ptr<cg::callgraph_t>
callgraph_create(int seed, const cfg::config &cf,
                 std::shared_ptr<tg::typegraph_t> sptg) {
  cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
  return std::make_shared<cg::callgraph_t>(std::move(newcf), sptg);
}

void callgraph_dump(std::shared_ptr<cg::callgraph_t> cg, std::ostream &os) {
  cg->dump(os);
}
