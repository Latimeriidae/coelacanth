//------------------------------------------------------------------------------
//
// Control-flow (top-level, before locations) for given functions
//
// split tree is slightly tricky
// it have labeled childs (order of childs in node is important)
// so boost graph is bad decision, and thus it is modeled by:
// (1) child list for each vertex
// (2) unordered map from vertex to its parent
// (3) unordered map from vertex to its bundled description
// (4) unordered set of vertices which are bbs, available to split
//
// Then typical split is trivial:
// * we are choosing random block from (3)
// * we are selecting its parent from (2)
// * we are making split in parents adjlist
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../dbgstream.h"
#include "callgraph/callgraph.h"
#include "controlgraph.h"
#include "typegraph/typegraph.h"
#include "varassign/varassign.h"

namespace cn {

using vct = std::vector<vertexprop_t>;
using vcit = typename vct::iterator;

class split_tree_t {
  cfg::config cf_;
  std::list<vertex_t> toplev_;
  std::vector<std::list<vertex_t>> adj_;
  std::unordered_map<vertex_t, vertex_t> parent_of_;
  std::unordered_map<vertex_t, vertexprop_t> desc_of_;
  std::unordered_set<vertex_t> bbs_;

public:
  split_tree_t(const cfg::config &cf, vcit start, vcit fin) : cf_(cf) {
    for (vcit cur = start; cur != fin; ++cur) {
      toplev_.push_back(adj_.size());
      adj_.emplace_back();
    }
  }

  vertex_iter_t begin() const { return toplev_.begin(); }
  vertex_iter_t end() const { return toplev_.end(); }

  // childs iterator
  vertex_iter_t begin_childs(vertex_t parent) const {
    return adj_[parent].begin();
  }
  vertex_iter_t end_childs(vertex_t parent) const { return adj_[parent].end(); }

  vertexprop_t from_vertex(vertex_t v) const {
    auto vit = desc_of_.find(v);
    if (vit == desc_of_.end())
      throw std::runtime_error("Vertex not found");
    return vit->second;
  }

  // TODO: tree-like print
  void dump(std::ostream &os) const {
    for (auto tl : toplev_) {
      os << tl << "\n";
    }
  }
};

void split_tree_deleter_t::operator()(split_tree_t *pst) { delete pst; }

//------------------------------------------------------------------------------
//
// Controlgraph public interface
//
//------------------------------------------------------------------------------

controlgraph_t::controlgraph_t(cfg::config &&cf,
                               std::shared_ptr<tg::typegraph_t> tgraph,
                               std::shared_ptr<cg::callgraph_t> cgraph,
                               std::shared_ptr<va::varassign_t> vassign)
    : config_(std::move(cf)), tgraph_(tgraph), cgraph_(cgraph),
      vassign_(vassign) {
  if (!config_.quiet())
    dbgs() << "Creating controlgraph\n";

  // for every call-graph function
  for (auto cgv : *cgraph_) {
    assert(int(cgv) < cgraph_->nfuncs());
    vct seeds;
    vertex_t id = 0;

    seeds.emplace_back(create_vprop<block_t>(id++));

    // initial seeds are direct calls
    for (auto cit = cgraph_->callees_begin(cgv, cg::calltype_t::DIRECT);
         cit != cgraph_->callees_end(cgv, cg::calltype_t::DIRECT); ++cit) {
      seeds.emplace_back(
          create_vprop<call_t>(id++, call_type_t::DIRECT, int(*cit)));
      seeds.emplace_back(create_vprop<block_t>(id++));
    }

    assert((id > 0) && (id == int(seeds.size())));

    // can not use make_unique here
    auto st = stt{new split_tree_t{config_, seeds.begin(), seeds.end()}};
    strees_.emplace_back(std::move(st));
  }
}

vertex_iter_t controlgraph_t::begin(int nfunc) const {
  return strees_[nfunc]->begin();
}

vertex_iter_t controlgraph_t::end(int nfunc) const {
  return strees_[nfunc]->end();
}

vertex_iter_t controlgraph_t::begin_childs(int nfunc, vertex_t parent) const {
  return strees_[nfunc]->begin_childs(parent);
}

vertex_iter_t controlgraph_t::end_childs(int nfunc, vertex_t parent) const {
  return strees_[nfunc]->end_childs(parent);
}

vertexprop_t controlgraph_t::from_vertex(int nfunc, vertex_t v) const {
  return strees_[nfunc]->from_vertex(v);
}

void controlgraph_t::dump(std::ostream &os) const {
  os << "Controlgraph consists of " << nfuncs() << " functions\n";
  for (auto &&t : strees_)
    t->dump(os);
}

} // namespace cn

//------------------------------------------------------------------------------
//
// Task system support
//
//------------------------------------------------------------------------------

std::shared_ptr<cn::controlgraph_t>
controlgraph_create(int seed, const cfg::config &cf,
                    std::shared_ptr<tg::typegraph_t> sptg,
                    std::shared_ptr<cg::callgraph_t> spcg,
                    std::shared_ptr<va::varassign_t> spva) {
  try {
    cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
    return std::make_shared<cn::controlgraph_t>(std::move(newcf), sptg, spcg,
                                                spva);
  } catch (std::runtime_error &e) {
    std::cerr << "Controlgraph construction problem: " << e.what() << std::endl;
    throw;
  }
}

void controlgraph_dump(std::shared_ptr<cn::controlgraph_t> pc,
                       std::ostream &os) {
  pc->dump(os);
}
