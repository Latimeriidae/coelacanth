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
#include <iterator>
#include <list>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "callgraph/callgraph.h"
#include "callgraph/calliters.h"
#include "coelacanth/dbgstream.h"
#include "controlgraph.h"
#include "splittree.h"
#include "typegraph/typegraph.h"
#include "varassign/varassign.h"

namespace cn {

//------------------------------------------------------------------------------
//
// Vertexprop helpers
//
//------------------------------------------------------------------------------

template <typename T, int N> constexpr int array_size(T (&)[N]) { return N; }

constexpr const char *blk_names[] = {"BLOCK",     "CALL",     "LOOP",
                                     "IF",        "SWITCH",   "REGION",
                                     "BRANCHING", "ACCBLOCK", "BREAKBLOCK"};

static_assert(array_size(blk_names) == int(category_t::CATMAX));

// NAME [special part] [defs] [uses]
std::ostream &operator<<(std::ostream &os, const vertexprop_t &v) {
  // NAME
  os << blk_names[int(v.cat())];

  // special part
  switch (v.cat()) {
  case category_t::BLOCK:
    break;
  case category_t::CALL: {
    call_t call = std::get<call_t>(v.type());
    switch (call.type) {
    case call_type_t::DIRECT:
      break;
    case call_type_t::INDIRECT:
      os << " IND";
      break;
    case call_type_t::CONDITIONAL:
      os << " COND";
      break;
    default:
      throw std::runtime_error("Unknown call type");
    }
    os << " TO FUNC #" << call.nfunc;
    break;
  }
  case category_t::LOOP: {
    loop_t loop = std::get<loop_t>(v.type());
    os << " from " << loop.start << " to " << loop.stop << " step "
       << loop.step;
    break;
  }
  case category_t::IF:
    break;
  case category_t::SWITCH:
    break;
  case category_t::REGION:
    break;
  case category_t::BRANCHING:
    break;
  case category_t::ACCESS:
    break;
  case category_t::BREAK: {
    break_t b = std::get<break_t>(v.type());
    switch (b.btp) {
    case break_type_t::CONTINUE:
      os << " [CONTINUE]";
      break;
    case break_type_t::BREAK:
      os << " [BREAK]";
      break;
    default:
      os << " [RETURN]";
      break;
    }
    break;
  }
  default:
    throw std::runtime_error("Unknown category");
  }

  // defs
  if (v.defs_begin() != v.defs_end()) {
    os << " DEFS:";
    for (auto it = v.defs_begin(); it != v.defs_end(); ++it)
      os << v.varname(*it) << " ";
  }

  // uses
  if (v.uses_begin() != v.uses_end()) {
    os << " USES:";
    for (auto it = v.uses_begin(); it != v.uses_end(); ++it)
      os << v.varname(*it) << " ";
  }

  return os;
}

std::string vertexprop_t::varname(va::variable_t v) const {
  return parent_.varname(v);
}

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

  strees_.resize(cgraph_->nfuncs());

  // for every call-graph function
  for (auto cgv : *cgraph_) {
    int cgvi = static_cast<int>(cgv);
    assert(cgvi < cgraph_->nfuncs());

    // can not use make_unique here (because we have custom deleter for stt)
    auto *pst = new split_tree_t{*this, config_, vassign_, cgvi};
    auto st = stt{pst};
    strees_[cgvi] = std::move(st);

    vct seeds;
    const split_tree_t &parent = *strees_[cgvi];

    seeds.emplace_back(create_vprop<block_t>(parent));

    // initial seeds are direct calls
    for (auto cit = cgraph_->callees_begin(cgv, cg::calltype_t::DIRECT);
         cit != cgraph_->callees_end(cgv, cg::calltype_t::DIRECT); ++cit) {
      seeds.emplace_back(
          create_vprop<call_t>(parent, call_type_t::DIRECT, int(*cit)));
      seeds.emplace_back(create_vprop<block_t>(parent));
    }

    strees_[cgvi]->process(seeds.begin(), seeds.end());
  }
}

int controlgraph_t::nfuncs() const { return cgraph_->nfuncs(); }

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

shared_vp_t controlgraph_t::from_vertex(int nfunc, vertex_t v) const {
  return strees_[nfunc]->from_vertex(v);
}

std::string controlgraph_t::varname(int nfunc, int vid) const {
  return vassign_->get_name(vid, nfunc);
}

int controlgraph_t::random_callee(int nfunc, call_type_t) const {
  // TODO: fix after callgraph support (shall be call-type related)
  cg::calltype_t mask = cg::calltype_t::CONDITIONAL;

  auto it = cgraph_->callees_begin(nfunc, mask);
  auto eit = cgraph_->callees_end(nfunc, mask);

  int dist = std::distance(it, eit);

  if (dist == 0)
    return -1;

  std::advance(it, config_.rand_positive() % dist);
  return *it;
}

void controlgraph_t::dump(std::ostream &os) const {
  os << "Controlgraph consists of " << nfuncs() << " functions\n";
  int n = 0;
  for (auto &&t : strees_) {
    os << "<FOO" << n++ << ">:" << std::endl;
    t->dump(os);
    os << "---" << std::endl << std::endl;
  }
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
    cfg::config newcf(seed, cf.quiet(), cf.dumps(), cf.cbegin(), cf.cend());
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
