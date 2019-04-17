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
#include <stack>
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

template <typename T, int N> constexpr int array_size(T (&)[N]) { return N; }

constexpr const char *blk_names[] = {"BLOCK",     "CALL",     "LOOP",
                                     "IF",        "SWITCH",   "REGION",
                                     "BRANCHING", "ACCBLOCK", "BREAKBLOCK"};

static_assert(array_size(blk_names) == int(category_t::CATMAX));

// NAME [special part] [defs] [uses]
std::ostream &operator<<(std::ostream &os, vertexprop_t v) {
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
  case category_t::LOOP:
    break;
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
  case category_t::BREAK:
    break;
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

using vct = std::vector<svp>;
using vcit = typename vct::iterator;

// split tree to create controlgraph structure
class split_tree_t {
  const controlgraph_t &parent_;

  // copy of config to possibly shake params individually
  cfg::config cf_;

  // number of function, this split tree is about in parent
  int nfunc_;

  // adjacency lists for vertices
  // we have special illegal vertex 0, adj list of which is
  // all top-level vertices
  static constexpr int PSEUDO_VERTEX = 0;

  std::vector<std::list<vertex_t>> adj_;
  std::unordered_map<vertex_t, vertex_t> parent_of_;
  std::unordered_map<vertex_t, svp> desc_of_;
  std::unordered_set<vertex_t> bbs_;

public:
  split_tree_t(const controlgraph_t &p, const cfg::config &cf, int nfunc)
      : parent_(p), cf_(cf), nfunc_(nfunc) {}

  void process(vcit start, vcit fin) {
    static_assert(PSEUDO_VERTEX == 0,
                  "We will have problems here if pseudo tp is not 0");
    assert(adj_.size() == 0 &&
           "We will have problems here if process called more than once");
    adj_.resize(fin - start + 1);
    int vidx = 1;
    parent_of_[PSEUDO_VERTEX] = ILLEGAL_VERTEX;
    desc_of_[PSEUDO_VERTEX] = nullptr;

    // initial seeds
    for (vcit cur = start; cur != fin; ++cur, ++vidx) {
      adj_[PSEUDO_VERTEX].push_back(vidx);
      desc_of_[vidx] = *cur;
      parent_of_[vidx] = PSEUDO_VERTEX;
      if ((*cur)->is_block())
        bbs_.insert(vidx);
    }

    // do splits
    int nsplits = cfg::get(cf_, MS::SPLITS);
    for (int i = 0; i < nsplits; ++i) {
      int navail = bbs_.size();
      auto bbit = bbs_.begin();
      std::advance(bbit, cf_.rand_positive() % navail);
      do_split(*bbit);
    }

    // assign variables
    // add accblocks
  }

  // toplevel iterator
  vertex_iter_t begin() const { return adj_[PSEUDO_VERTEX].begin(); }
  vertex_iter_t end() const { return adj_[PSEUDO_VERTEX].end(); }

  // childs iterator
  vertex_iter_t begin_childs(vertex_t parent) const {
    return adj_[parent].begin();
  }
  vertex_iter_t end_childs(vertex_t parent) const { return adj_[parent].end(); }

  svp from_vertex(vertex_t v) const {
    auto vit = desc_of_.find(v);
    if (vit == desc_of_.end())
      throw std::runtime_error("Vertex not found");
    return vit->second;
  }

  std::string varname(va::variable_t v) const {
    return parent_.varname(nfunc_, v.id);
  }

  // tree-like print of controlgraph
  void dump(std::ostream &os) const {
    using stackelem_t = std::pair<int, vertex_t>;
    std::stack<stackelem_t> s;
    for (auto tl : adj_[PSEUDO_VERTEX])
      s.push(std::make_pair(0, tl));
    while (!s.empty()) {
      auto [level, nvert] = s.top();
      s.pop();

      for (int i = 0; i < level; i++)
        os << " ";
      os << *from_vertex(nvert) << "\n";

      for (auto chvert : adj_[nvert])
        s.push(std::make_pair(level + 2, chvert));
    }
  }

private:
  template <typename It> It add_block(It pos, vertex_t parent) {
    int nblock = adj_.size();
    assert(parent < nblock);
    adj_.emplace_back();
    svp prop = create_vprop<block_t>(*this, nblock);

    It ret;
    if (adj_[parent].empty()) {
      adj_[parent].push_back(nblock);
      ret = adj_[parent].begin();
    } else
      ret = adj_[parent].insert(pos, nblock);

    desc_of_[nblock] = prop;
    parent_of_[nblock] = parent;
    bbs_.insert(nblock);
    return ret;
  }

  // turns anything at nblock into T
  template <typename T, typename... Args>
  void turn_block(int nblock, Args &&... args) {
    desc_of_[nblock] =
        create_vprop<T>(*this, nblock, std::forward<Args>(args)...);
    if constexpr (T::cat != category_t::BLOCK) {
      bbs_.erase(nblock);
    } else {
      bbs_.insert(nblock);
    }
  }

  void do_split(int bb_under_split) {
    assert(bb_under_split != PSEUDO_VERTEX);
    assert(parent_of_.find(bb_under_split) != parent_of_.end());

    // 1. find position before this block in list of childs of its parent
    int nbbp = parent_of_[bb_under_split];

    auto nbit = adj_[nbbp].begin();
    for (; nbit != adj_[nbbp].end(); ++nbit)
      if (*nbit == bb_under_split)
        break;

    if (nbit == adj_[nbbp].end())
      throw std::runtime_error("Not found in list of childs of its parent");

    // 2. add several more blocks
    int naddblocks = cfg::get(cf_, CN::ADDBLOCKS);
    if (naddblocks > 0) {
      auto nbnext = nbit;
      for (int i = 0; i < naddblocks; ++i)
        nbnext = add_block(nbnext, nbbp);
      std::advance(nbit, cf_.rand_positive() % naddblocks);
      bb_under_split = *nbit;
    }

    // 3. Either:
    //   3.1 turn block into container and add childs
    //   3.2 turn block into special block
    if (cfg::get(cf_, CN::EXPANDCONT)) {
      int nchilds = 1;
      int cont_type = cfg::get(cf_, CN::CONTPROB);
      switch (cont_type) {
      case CNC_IF:
        turn_block<if_t>(bb_under_split);
        nchilds = cfg::get(cf_, CN::NBRANCHES_IF);
        break;
      case CNC_FOR:
        turn_block<loop_t>(bb_under_split);
        break;
      case CNC_SWITCH:
        turn_block<switch_t>(bb_under_split);
        nchilds = cfg::get(cf_, CN::NBRANCHES_IF);
        break;
      case CNC_REGION:
        turn_block<region_t>(bb_under_split);
        nchilds = cfg::get(cf_, CN::NBRANCHES_IF);
        break;
      default:
        throw std::runtime_error("Unknown container");
      }
      std::stack<int> create_childs;
      if (desc_of_[bb_under_split]->is_branching()) {
        for (int i = 0; i < nchilds; ++i) {
          auto nb = add_block(adj_[bb_under_split].begin(), bb_under_split);
          turn_block<branching_t>(*nb);
          create_childs.push(*nb);
        }
      } else
        create_childs.push(bb_under_split);
      while (!create_childs.empty()) {
        int pbb = create_childs.top();
        assert(pbb < int(adj_.size()));
        create_childs.pop();
        add_block(adj_[pbb].begin(), pbb);
      }
    } else {
      int block_type = cfg::get(cf_, CN::BLOCKPROB);
      switch (block_type) {
      case CNB_BREAK:
        break;
      case CNB_CCALL:
        break;
      case CNB_ICALL:
        break;
      default:
        throw std::runtime_error("Unknown block");
      }
    }
  }
};

void split_tree_deleter_t::operator()(split_tree_t *pst) { delete pst; }

// this needs to be here becuase parent_ shall be full type
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
    auto *pst = new split_tree_t{*this, config_, cgvi};
    auto st = stt{pst};
    strees_[cgvi] = std::move(st);

    vct seeds;
    vertex_t id = 0;
    const split_tree_t &parent = *strees_[cgvi];

    seeds.emplace_back(create_vprop<block_t>(parent, id++));

    // initial seeds are direct calls
    for (auto cit = cgraph_->callees_begin(cgv, cg::calltype_t::DIRECT);
         cit != cgraph_->callees_end(cgv, cg::calltype_t::DIRECT); ++cit) {
      seeds.emplace_back(
          create_vprop<call_t>(parent, id++, call_type_t::DIRECT, int(*cit)));
      seeds.emplace_back(create_vprop<block_t>(parent, id++));
    }

    assert((id > 0) && (id == int(seeds.size())));
    strees_[cgvi]->process(seeds.begin(), seeds.end());
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

svp controlgraph_t::from_vertex(int nfunc, vertex_t v) const {
  return strees_[nfunc]->from_vertex(v);
}

std::string controlgraph_t::varname(int nfunc, int vid) const {
  return vassign_->get_name(vid, nfunc);
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
