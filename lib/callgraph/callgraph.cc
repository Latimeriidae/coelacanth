//------------------------------------------------------------------------------
//
// Callgraph impl
//
// ctor sequence is:
// 1. generate random graph
// 2. add more leafs to non-leaf nodes
// 3. connect components (to remove unreachable nodes)
// 4. set self-loops
// 5. create indirect sets
// 6. assign function and return types
// 7. modules affinity
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <queue>
#include <set>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

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

std::string vertexprop_t::get_color() const {
  if (indset != 0)
    return "blue";
  return "black";
}

std::string edgeprop_t::get_style() const {
  if (calltype == calltype_t::INDIRECT)
    return "dashed";
  return "solid";
}

std::string edgeprop_t::get_color() const {
  if (calltype == calltype_t::DIRECT)
    return "red";
  return "black";
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
// Edge visitor to create spanning tree
//
//------------------------------------------------------------------------------

struct bfs_edge_visitor : public boost::default_bfs_visitor {
  void tree_edge(edge_t e, const cgraph_t &cg) const {
    cgraph_t &g = const_cast<cgraph_t &>(cg);
    auto &eprop = boost::get(boost::edge_bundle, g, e);
    eprop.calltype = calltype_t::DIRECT;
  }
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
  int nvertices = cfg::get(config_, CG::VERTICES);
  int nedgeset = cfg::get(config_, CG::EDGESET);
  generate_random_graph(nvertices, nedgeset);

  // partition to leafs and non-leafs and add more leafs
  process_leafs();

  // connecting components
  connect_components();

  // adding possible self-loops
  // We can not do it earlier, self-loops will break in-degree analysis
  // in components connection function
  add_self_loops();

  // --- here graph structure is frozen ---

  // create indirect calls set
  create_indcalls();

  // decide on high-level metastructure
  decide_metastructure();

  // assign function and return types
  assign_types();

  // modules affinity
  map_modules();
}

// dump as dot file
void callgraph_t::dump(std::ostream &os) const {
  boost::dynamic_properties dp;
  auto vbundle = boost::get(boost::vertex_bundle, graph_);
  dp.property("node_id", boost::get(boost::vertex_index, graph_));
  dp.property("label", boost::make_transform_value_property_map(
                           std::mem_fn(&vertexprop_t::get_name), vbundle));
  dp.property("color", boost::make_transform_value_property_map(
                           std::mem_fn(&vertexprop_t::get_color), vbundle));

  auto ebundle = boost::get(boost::edge_bundle, graph_);
  dp.property("style", boost::make_transform_value_property_map(
                           std::mem_fn(&edgeprop_t::get_style), ebundle));
  dp.property("color", boost::make_transform_value_property_map(
                           std::mem_fn(&edgeprop_t::get_color), ebundle));

  boost::write_graphviz_dp(os, graph_, dp);
}

//------------------------------------------------------------------------------
//
// Callgraph construction helpers
//
//------------------------------------------------------------------------------

// I tried boost::generate_random_graph
// I also tried boost::erdos_renyi_iterator
// none served well enough, so fall back to custom method
void callgraph_t::generate_random_graph(int nvertices, int nedgeset) {
  for (int i = 0; i < nvertices; ++i) {
    auto v = boost::add_vertex(graph_);
    graph_[v].funcid = v;
  }

  // TODO:
  // To make call graph more interesting we may want to generate strongly
  // coupled internally components, loosely coupled pairwise
  // We may think about some option, like CG::LOOSE_COMPONENTS (0/1)

  // we do not want to allow self-loops on this stage
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    for (auto [vi2, vi2_end] = boost::vertices(graph_); vi2 != vi2_end; ++vi2)
      if ((vi != vi2) && ((config_.rand_positive() % 100) < nedgeset))
        boost::add_edge(*vi, *vi2, graph_);

  // TODO:
  // To make graph more realistic it makes sense to create option to control
  // max length of the longest path in the graph (like CG::MAXPATH)
  // Say we can prune long paths by inverting edges in-between

  int nheads = 0;
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    if (vertex_t v = *vi; boost::in_degree(v, graph_) == 0)
      nheads += 1;

  // highly unprobable case: no vertex have zero in-degree
  if (0 == nheads) {
    std::vector<vertex_t> conns;
    config_rng cfrng(config_);
    auto [vi, vi_end] = boost::vertices(graph_);
    int nconns = cfg::get(config_, CG::ARTIFICIAL_CONNS);
    std::sample(vi, vi_end, std::back_inserter(conns), nconns,
                std::move(cfrng));

    auto v = boost::add_vertex(graph_);
    graph_[v].funcid = v;
    for (auto vc : conns)
      boost::add_edge(v, vc, graph_);
  }
}

void callgraph_t::process_leafs() {
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    vertex_t v = *vi;
    if (boost::out_degree(v, graph_) == 0)
      leafs_.insert(v);
    else
      non_leafs_.insert(v);
  }
  assert(!non_leafs_.empty() && "Graph have no non-leafs?");

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
}

// TODO:
// component connection is not ideal
// consider graph like {A, B, C, D, B->C, C->D, D->B}
// it will find only {A} and set A to be main, leaving {B, C, D} dead code
// but it is already overly complicated and in most cases it works
// so let it be
void callgraph_t::connect_components() {
  auto n = boost::num_vertices(graph_);
  std::vector<int> rank_map(n);
  std::vector<vertex_t> pred_map(n);

  auto rank = make_iterator_property_map(
      rank_map.begin(), boost::get(boost::vertex_index, graph_), rank_map[0]);
  auto parent = make_iterator_property_map(
      pred_map.begin(), boost::get(boost::vertex_index, graph_), pred_map[0]);
  boost::disjoint_sets dset(rank, parent);

  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    dset.make_set(*vi);

  for (auto [ei, eiend] = boost::edges(graph_); ei != eiend; ++ei) {
    edge_t e = *ei;
    vertex_t u = dset.find_set(boost::source(e, graph_));
    vertex_t v = dset.find_set(boost::target(e, graph_));
    if (u != v)
      dset.link(u, v);
  }

  auto [vi, vi_end] = boost::vertices(graph_);
  dset.compress_sets(vi, vi_end);

  std::map<vertex_t, std::vector<vertex_t>> heads;
  std::vector<vertex_t> hreps_vector;

  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    if (vertex_t v = *vi; boost::in_degree(v, graph_) == 0)
      heads[dset.find_set(v)].push_back(v);

  // heads can not be empty by construction
  assert(!heads.empty());

  // store keys to representatives vector
  boost::copy(heads | boost::adaptors::map_keys,
              std::back_inserter(hreps_vector));

  // sort representatives vector by component size backwards
  std::sort(hreps_vector.begin(), hreps_vector.end(),
            [heads](vertex_t u, vertex_t v) {
              return heads.find(u)->second.size() >
                     heads.find(v)->second.size();
            });

  // edges from component head to other heads
  for (auto rep : hreps_vector) {
    auto &&hvec = heads[rep];
    vertex_t vtop = hvec[0];
    for (auto h : hvec) {
      if (h == vtop)
        continue;
      boost::add_edge(vtop, h, graph_);
    }

    // setting component heads
    comps_.push_back({});
    size_t ncomp = comps_.size() - 1;
    comps_[ncomp].push_back(vtop);
    graph_[vtop].componentno = ncomp;
  }

  // distributing functions over components
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    for (auto &comp : comps_)
      if (vertex_t v = *vi;
          (v != comp[0]) && (dset.find_set(v) == dset.find_set(comp[0]))) {
        comp.push_back(v);
        graph_[v].componentno = std::distance(&comps_[0], &comp);
      }

      // main function is now comps_[0][0]
#if 0
  std::cout << "Connected components:" << std::endl;
  for (const auto &c : comps_) {
    for (auto celt : c)
      std::cout << celt << " ";
    std::cout << std::endl;
  }
#endif
}

void callgraph_t::add_self_loops() {
  int main_head = comps_[0][0];

  int nselfloop = cfg::get(config_, CG::SELFLOOP);
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    if ((config_.rand_positive() % 100) < nselfloop)
      boost::add_edge(*vi, *vi, graph_);

  // setting direct calls
  boost::breadth_first_search(graph_, main_head,
                              boost::visitor(bfs_edge_visitor{}));
}

void callgraph_t::create_indcalls() {
  config_rng cfrng(config_);
  int nindirect = cfg::get(config_, CG::INDSETCNT);
  if ((comps_.size() > 1) && (nindirect > 0))
    for (auto it = comps_.begin() + 1; it != comps_.end(); ++it)
      for (auto vi : *it) {
        nindirect -= 1;
        inds_.push_back(vi);
        if (nindirect == 0)
          break;
      }

  if (nindirect > 0)
    std::sample(comps_[0].begin() + 1, comps_[0].end(),
                std::back_inserter(inds_), nindirect, std::move(cfrng));

  // indset zero is not indirect at all
  // TODO: may be we will want SEVERAL indirect sets: 2, 3, ....?
  for (auto iv : inds_)
    graph_[iv].indset = 1;

#if 0
  std::cout << "Chosen to be indirectly callable" << std::endl;
  for (auto iv : inds_)
    std::cout << iv << " ";
  std::cout << std::endl;
#endif
}

void callgraph_t::decide_metastructure() {
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    vertexprop_t &vpt = graph_[*vi];
    vpt.usesigned =
        (config_.rand_positive() % 100) < cfg::get(config_, MS::USESIGNED) ? 1
                                                                           : 0;
    vpt.usefloat =
        (config_.rand_positive() % 100) < cfg::get(config_, MS::USEFLOAT) ? 1
                                                                          : 0;
    vpt.usecomplex =
        (config_.rand_positive() % 100) < cfg::get(config_, MS::USECOMPLEX) ? 1
                                                                            : 0;
    vpt.usepointers =
        (config_.rand_positive() % 100) < cfg::get(config_, MS::USEPOINTERS)
            ? 1
            : 0;
  }
}

void callgraph_t::assign_types() {
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
  }
}

void callgraph_t::map_modules() {}

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
