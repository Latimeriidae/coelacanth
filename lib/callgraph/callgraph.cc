//------------------------------------------------------------------------------
//
// Callgraph impl
//
// ctor sequence is:
// 1. generate random graph
// 2. add more leafs to non-leaf nodes
// 3. calculate spanning arborescence for each top-level node
// 4. add component root vertices
// 5. create indirect sets
// 6. assign function and return types
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>
#include <queue>
#include <set>

#include <boost/graph/graphviz.hpp>
#include <boost/graph/random.hpp>
#include <boost/pending/disjoint_sets.hpp>

#include "callgraph.h"
#include "typegraph/typean.h"
#include "typegraph/typegraph.h"

namespace cg {

// label for dot dump of callgraph
std::string vertexprop_t::get_name() const {
  std::ostringstream s;
  s << "foo" << funcid;
  return s.str();
}

//------------------------------------------------------------------------------
//
// Config adapter to make rng out of rng, hidden in cfg
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

//------------------------------------------------------------------------------
//
// Callgraph public interface
//
//------------------------------------------------------------------------------

// ctor is the only public modifying function in callgraph
// see construction sequence description in head comment
callgraph_t::callgraph_t(cfg::config &&cf,
                         std::shared_ptr<tg::typegraph_t> tgraph)
    : config_(std::move(cf)), tgraph_(tgraph) {
  if (!config_.quiet())
    std::cout << "Creating callgraph" << std::endl;

  // generate random graph
  config_rng cfrng(config_);
  int nvertices = cfg::get(config_, CG::VERTICES);
  int nedges = cfg::get(config_, CG::EDGES);
  boost::generate_random_graph(graph_, nvertices, nedges, cfrng,
                               /* par */ false, /* self */ true);

  // add more leafs to non-leaf nodes
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    vertex_t v = *vi;
    graph_[v].funcid = v;
    int odeg = boost::out_degree(v, graph_);
    if (odeg == 0)
      leafs_.insert(v);
    else
      non_leafs_.insert(v);
  }
  assert(!non_leafs_.empty());

  int naddleafs = cfg::get(config_, CG::ADDLEAFS);
  for (int i = 0; i < naddleafs; ++i) {
    auto it = non_leafs_.begin();
    int n = config_.rand_positive() % non_leafs_.size();
    std::advance(it, n);

    auto sv = boost::add_vertex(graph_);
    graph_[sv].funcid = sv;
    leafs_.insert(sv);
    boost::add_edge(*it, sv, graph_);
  }

  // calculate spanning arborescence for each top-level node
  connect_components();

  // add component root vertices

  // create indirect sets

  // --- here graph structure is frozen ---

  // assign function and return types
}

// dump as dot file
void callgraph_t::dump(std::ostream &os) const {
  boost::dynamic_properties dp;
  auto bundle = boost::get(boost::vertex_bundle, graph_);
  dp.property("node_id", boost::get(boost::vertex_index, graph_));
  dp.property("label", boost::make_transform_value_property_map(
                           std::mem_fn(&vertexprop_t::get_name), bundle));
  boost::write_graphviz_dp(os, graph_, dp);
}

//------------------------------------------------------------------------------
//
// Callgraph construction helpers
//
//------------------------------------------------------------------------------

void callgraph_t::connect_components() {
  auto n = boost::num_vertices(graph_);
  std::vector<int> rank_map(n);
  std::vector<vertex_t> pred_map(n);
  std::queue<edge_t> edges_queue;
  std::vector<vertex_t> heads;

  auto rank = make_iterator_property_map(
      rank_map.begin(), boost::get(boost::vertex_index, graph_), rank_map[0]);
  auto parent = make_iterator_property_map(
      pred_map.begin(), boost::get(boost::vertex_index, graph_), pred_map[0]);
  boost::disjoint_sets dset(rank, parent);

  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    dset.make_set(*vi);

  {
    auto [vi, vi_end] = boost::vertices(graph_);
    std::cout << "Sets (" << dset.count_sets(vi, vi_end) << " total)"
              << std::endl;
  }

  for (auto [ei, eiend] = boost::edges(graph_); ei != eiend; ++ei) {
    edge_t e = *ei;
    vertex_t u = dset.find_set(boost::source(e, graph_));
    vertex_t v = dset.find_set(boost::target(e, graph_));
    if (u != v)
      dset.link(u, v);
  }

  {
    auto [vi, vi_end] = boost::vertices(graph_);
    std::cout << "Sets (" << dset.count_sets(vi, vi_end) << " total)"
              << std::endl;
  }

  {
    auto [vi, vi_end] = boost::vertices(graph_);
    dset.compress_sets(vi, vi_end);
  }

#if 0
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    vertex_t v = *vi;
    vertex_t u = dset.find_set(v);
    std::cout << v << " represented by " << u << std::endl;
  }
#endif

  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    if (boost::in_degree(*vi, graph_) == 0)
      heads.push_back(*vi);

  // best but highly unprobable case: single connected component, no vertex have
  // zero in-degree add head vertex, connect with random one(s) and return
  if (heads.empty()) {
    // TODO: support
    throw std::runtime_error("Sorry, unimplemented");
  }
}

} // namespace cg

//------------------------------------------------------------------------------
//
// Task system support
//
//------------------------------------------------------------------------------

std::shared_ptr<cg::callgraph_t>
callgraph_create(int seed, const cfg::config &cf,
                 std::shared_ptr<tg::typegraph_t> sptg) {
  cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
  return std::make_shared<cg::callgraph_t>(std::move(newcf), sptg);
}

void callgraph_dump(std::shared_ptr<cg::callgraph_t> cg, std::ostream &os) {
  cg->dump(os);
}
