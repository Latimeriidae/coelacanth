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
// assignment of types to functions rules:
// - function can accept and return only full metastructure-according scalar
// - for complex types only partial accordnance required
//   * example: struct {unsigned x, float y} foo1() when foo1 is non-float
//   *          in this case y member returns value-unitialized
// - we are doing CG::TYPEATTEMPTS attempts to assign random type
//   * then we are traversing type storage and assigning first suitable
// - also return and argument type can not be array_t
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
#include "funcmeta.h"
#include "typegraph/typean.h"
#include "typegraph/typegraph.h"

namespace cg {

// label for dot dump of callgraph
std::string
vertexprop_t::get_name(std::shared_ptr<tg::typegraph_t> tgraph) const {
  std::ostringstream s;
  if (rettype == -1)
    s << "void";
  else
    s << tgraph->vertex_from(rettype).get_short_name();

  s << " foo" << funcid << "(";
  for (auto ait = argtypes.begin(); ait != argtypes.end(); ++ait) {
    if (ait != argtypes.begin())
      s << ", ";
    s << tgraph->vertex_from(*ait).get_short_name();
  }
  s << ")";
  return s.str();
}

std::string vertexprop_t::get_color() const {
  if (indset != 0)
    return "blue";
  return "black";
}

std::string edgeprop_t::get_style() const { return "solid"; }

std::string edgeprop_t::get_color() const {
  if (calltype == calltype_t::DIRECT)
    return "red";
  return "black";
}

vertexprop_t vertex_from(const callgraph_t *pcg, vertex_t v) {
  return pcg->vertex_from(v);
}

vertex_t dest_from(const callgraph_t *pcg, edge_t e) {
  return pcg->dest_from(e);
}

vertex_t src_from(const callgraph_t *pcg, edge_t e) { return pcg->src_from(e); }

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
  generate_random_graph(nvertices);

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

vertex_iter_t callgraph_t::begin() const {
  auto [vi, vi_end] = boost::vertices(graph_);
  return vi;
}

vertex_iter_t callgraph_t::end() const {
  auto [vi, vi_end] = boost::vertices(graph_);
  return vi;
}

callee_iterator_t callgraph_t::callees_begin(vertex_t v,
                                             calltype_t mask) const {
  auto [ei, ei_end] = boost::out_edges(v, graph_);
  return callee_iterator_t(this, ei, mask);
}

callee_iterator_t callgraph_t::callees_end(vertex_t v, calltype_t mask) const {
  auto [ei, ei_end] = boost::out_edges(v, graph_);
  return callee_iterator_t(this, ei_end, mask);
}

caller_iterator_t callgraph_t::callers_begin(vertex_t v,
                                             calltype_t mask) const {
  auto [ei, ei_end] = boost::in_edges(v, graph_);
  return caller_iterator_t(this, ei, mask);
}

caller_iterator_t callgraph_t::callers_end(vertex_t v, calltype_t mask) const {
  auto [ei, ei_end] = boost::in_edges(v, graph_);
  return caller_iterator_t(this, ei_end, mask);
}

// dump as dot file
void callgraph_t::dump(std::ostream &os) const {
  boost::dynamic_properties dp;
  auto vbundle = boost::get(boost::vertex_bundle, graph_);
  dp.property("node_id", boost::get(boost::vertex_index, graph_));

  // std::mem_fn(&vertexprop_t::get_name)
  auto cg_name = [this](vertexprop_t v) { return v.get_name(tgraph_); };

  dp.property("label",
              boost::make_transform_value_property_map(cg_name, vbundle));
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
void callgraph_t::generate_random_graph(int nvertices) {
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
      if ((vi != vi2) && cfg::get(config_, CG::EDGESET))
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
    cfg::config_rng cfrng(config_);
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

  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi)
    if (cfg::get(config_, CG::SELFLOOP))
      boost::add_edge(*vi, *vi, graph_);

  // setting direct calls
  boost::breadth_first_search(graph_, main_head,
                              boost::visitor(bfs_edge_visitor{}));
}

void callgraph_t::create_indcalls() {
  cfg::config_rng cfrng(config_);
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
  // for all indirect calls same metastructure
  auto ind_metainfo = ms::random_meta(config_);

  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    vertexprop_t &vpt = graph_[*vi];
    if (vpt.indset)
      vpt.metainfo = ind_metainfo;
    else
      vpt.metainfo = ms::random_meta(config_);
#if 0
    std::cout << "For function " << *vi << " config is: " << vpt.usesigned
              << " " << vpt.usefloat << " " << vpt.usecomplex << " "
              << vpt.usepointers << std::endl;
#endif
  }
}

int callgraph_t::pick_typeid(vertex_t v, bool allow_void, bool ret_type) {
  vertexprop_t &vp = graph_[v];
  bool succ = false;
  for (int nattempts = cfg::get(config_, CG::TYPEATTEMPTS); nattempts > 0;
       --nattempts) {
    auto randt = tgraph_->get_random_type();
    if ((randt.cat == tg::category_t::ARRAY) && ret_type)
      continue;
    if (ms::check_type(vp.metainfo, randt)) {
      succ = true;
      return randt.id;
    }
  }

  // corner case: random type selection failed (this may eventually happen if
  // metastructure is too restrictive)
  if (!succ) {
    for (auto tv = tgraph_->begin(), tve = tgraph_->end(); tv != tve; ++tv) {
      auto randt = tgraph_->vertex_from(*tv);
      if ((randt.cat == tg::category_t::ARRAY) && ret_type)
        continue;
      if (ms::check_type(vp.metainfo, randt)) {
        succ = true;
        return randt.id;
      }
    }
  }

  if (!allow_void)
    throw std::runtime_error("Can not find type in typestorage to conform");

  return -1;
}

std::pair<int, std::vector<int>> callgraph_t::gen_params(vertex_t v) {
  int rettype = pick_typeid(v, true, true);
  int nargs = cfg::get(config_, CG::NARGS);
  std::vector<int> args;
  if (nargs > 0)
    args.resize(nargs);
  for (auto &at : args)
    at = pick_typeid(v);
  return make_pair(rettype, args);
}

void callgraph_t::assign_types() {
  // for all indirect calls pre-generate
  int indret = -1;
  std::vector<int> indargs;
  if (inds_.size() > 0)
    std::tie(indret, indargs) = gen_params(inds_[0]);

  // for others
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    vertex_t v = *vi;
    vertexprop_t &vp = graph_[v];
    if (!vp.indset)
      std::tie(vp.rettype, vp.argtypes) = gen_params(v);
    else {
      vp.rettype = indret;
      vp.argtypes = indargs;
    }
  }
}

void callgraph_t::map_modules() {}

} // namespace cg

//------------------------------------------------------------------------------
//
// Metastructure
//
//------------------------------------------------------------------------------

namespace ms {

metanode_t random_meta(const cfg::config &config) {
  metanode_t ret;
  ret.usesigned = cfg::get(config, MS::USESIGNED);
  ret.usefloat = cfg::get(config, MS::USEFLOAT);
  ret.usecomplex = cfg::get(config, MS::USECOMPLEX);
  ret.usepointers = cfg::get(config, MS::USEPOINTERS);
  return ret;
}

bool check_type(metanode_t m, tg::vertexprop_t vpt) {
  switch (vpt.cat) {
  case tg::category_t::SCALAR: {
    tg::scalar_t s = std::get<tg::scalar_t>(vpt.type);
    if (s.sdesc->is_float && !m.usefloat)
      return false;
    if (s.sdesc->is_signed && !m.usesigned)
      return false;
    break;
  }
  case tg::category_t::STRUCT:
    if (!m.usecomplex)
      return false;
    break;
  case tg::category_t::ARRAY:
    if (!m.usecomplex)
      return false;
    break;
  case tg::category_t::POINTER:
    if (!m.usepointers)
      return false;
    break;
  default:
    throw std::runtime_error("Unknown cat");
  };

  return true;
}

} // namespace ms

//------------------------------------------------------------------------------
//
// Task system support
//
//------------------------------------------------------------------------------

std::shared_ptr<cg::callgraph_t>
callgraph_create(int seed, const cfg::config &cf,
                 std::shared_ptr<tg::typegraph_t> sptg) {
  try {
    cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
    return std::make_shared<cg::callgraph_t>(std::move(newcf), sptg);
  } catch (std::runtime_error &e) {
    std::cerr << "Callgraph construction problem: " << e.what() << std::endl;
    throw;
  }
}

void callgraph_dump(std::shared_ptr<cg::callgraph_t> cg, std::ostream &os) {
  cg->dump(os);
}
