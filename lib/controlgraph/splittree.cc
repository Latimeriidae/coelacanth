//------------------------------------------------------------------------------
//
// Splittree: special labeled tree for internal controlgraph usage impl
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <cassert>

#include "controlgraph.h"
#include "controltypes.h"
#include "splittree.h"
#include "varassign/varassign.h"

namespace cn {

split_tree_t::split_tree_t(const controlgraph_t &p, const cfg::config &cf,
                           std::shared_ptr<va::varassign_t> va, int nfunc)
    : parent_(p), cf_(cf), vassign_(va), nfunc_(nfunc) {}

void split_tree_t::process(vcit start, vcit fin) {
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
  for (size_t i = 1; i < adj_.size(); ++i)
    assign_vars_to(desc_of_[i]);

  // add accblocks
  // THIS -> CHILDS into THIS -> ACCS, ACC -> CHILD
  size_t cursize = adj_.size();
  for (size_t i = 1; i < cursize; ++i) {
    // for all childs of current block
    // counting accs
    // creating accblock with index vars
    // making child into child of accblock
    // replacing child with its accblock in parent
  }
}

// vertex property from desc
shared_vp_t split_tree_t::from_vertex(vertex_t v) const {
  auto vit = desc_of_.find(v);
  if (vit == desc_of_.end())
    throw std::runtime_error("Vertex not found");
  return vit->second;
}

// variable name from desc
std::string split_tree_t::varname(va::variable_t v) const {
  return parent_.varname(nfunc_, v.id);
}

// tree-like print of controlgraph
void split_tree_t::dump(std::ostream &os) const {
  using stackelem_t = std::pair<int, vertex_t>;
  std::stack<stackelem_t> s;
  for (auto ri = adj_[PSEUDO_VERTEX].rbegin(), re = adj_[PSEUDO_VERTEX].rend();
       ri != re; ++ri)
    s.push(std::make_pair(0, *ri));
  while (!s.empty()) {
    auto [level, nvert] = s.top();
    s.pop();

    for (int i = 0; i < level; i++)
      os << " ";
    os << *from_vertex(nvert) << "\n";

    for (auto ri = adj_[nvert].rbegin(), re = adj_[nvert].rend(); ri != re;
         ++ri)
      s.push(std::make_pair(level + 2, *ri));
  }
}

// add block to list of childs at pos, return iterator
itpos_t split_tree_t::add_block(itpos_t pos, vertex_t parent) {
  int nblock = adj_.size();
  assert(parent < nblock);
  adj_.emplace_back();
  shared_vp_t prop = create_vprop<block_t>(*this, nblock);

  itpos_t ret;
  if (adj_[parent].empty()) {
    adj_[parent].push_back(nblock);
    ret = adj_[parent].begin();
  } else {
    // insert after
    assert(pos != adj_[parent].end());
    ret = adj_[parent].insert(++pos, nblock);
  }

  desc_of_[nblock] = prop;
  parent_of_[nblock] = parent;
  bbs_.insert(nblock);
  return ret;
}

// add container and childs to it
void split_tree_t::add_container(int bb_under_split) {
  int nchilds = 1;
  int cont_type = cfg::get(cf_, CN::CONTPROB);
  switch (cont_type) {
  case CNC_IF:
    turn_block<if_t>(bb_under_split);
    nchilds = cfg::get(cf_, CN::NBRANCHES_IF);
    break;
  case CNC_FOR: {
    int start = cfg::get(cf_, CN::FOR_START);
    int stop = start + cfg::get(cf_, CN::FOR_SIZE);
    int step = cfg::get(cf_, CN::FOR_STEP);
    turn_block<loop_t>(bb_under_split, start, stop, step);
    break;
  }
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
  // need to reserve since all contents and iterators
  // to it (including list iterators) can be invalidated
  adj_.reserve(adj_.size() + nchilds);
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
}

// does bb have pcat among his parents?
bool split_tree_t::have_parent(int bb, category_t pcat) const {
  shared_vp_t cur;

  do {
    auto pit = parent_of_.find(bb);
    assert(pit != parent_of_.end());

    bb = pit->second;
    if (bb == PSEUDO_VERTEX)
      break;
    cur = from_vertex(bb);
  } while (cur->cat() != pcat);

  return (bb != PSEUDO_VERTEX);
}

// add special node like break or call
void split_tree_t::add_special(int bb_under_split) {
  int block_type = cfg::get(cf_, CN::BLOCKPROB);
  switch (block_type) {
  case CNB_BREAK: {
    break_type_t btp = break_type_t::RETURN;

    // Determine can it be break/continue or return only
    if (have_parent(bb_under_split, category_t::LOOP)) {
      int nbt = cfg::get(cf_, CN::BREAKTYPE);
      switch (nbt) {
      case CNBR_BREAK:
        btp = break_type_t::BREAK;
        break;
      case CNBR_CONT:
        btp = break_type_t::CONTINUE;
        break;
      default:; // do nothing, btp already return
      }
    }

    turn_block<break_t>(bb_under_split, btp);
    break;
  }
  case CNB_CCALL:
  case CNB_ICALL: {
    call_type_t ctp = call_type_t::INDIRECT;
    if (CNB_CCALL == block_type)
      ctp = call_type_t::CONDITIONAL;
    int ncallee = parent_.random_callee(nfunc_, ctp);
    if (-1 != ncallee)
      turn_block<call_t>(bb_under_split, ctp, ncallee);
    break;
  }
  default:
    throw std::runtime_error("Unknown block");
  }
}

void split_tree_t::do_split(int bb_under_split) {
  assert(bb_under_split != PSEUDO_VERTEX);
  assert(parent_of_.find(bb_under_split) != parent_of_.end());

  int naddblocks = cfg::get(cf_, CN::ADDBLOCKS);
  // need to reserve since all contents and iterators
  // to it (including list iterators) can be invalidated
  adj_.reserve(adj_.size() + naddblocks);

  // 1. find position before this block in list of childs of its parent
  int nbbp = parent_of_[bb_under_split];

  auto nbit = adj_[nbbp].begin();
  for (; nbit != adj_[nbbp].end(); ++nbit)
    if (*nbit == bb_under_split)
      break;

  if (nbit == adj_[nbbp].end())
    throw std::runtime_error("Not found in list of childs of its parent");

  // 2. add several more blocks
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
  if (cfg::get(cf_, CN::EXPANDCONT))
    add_container(bb_under_split);
  else
    add_special(bb_under_split);
}

void split_tree_t::add_vars(int cntp, shared_vp_t svp) {
  auto vars_begin = vassign_->fv_begin(nfunc_);
  auto vars_end = vassign_->fv_end(nfunc_);
  int nvars = vars_end - vars_begin;

  int nuds = cfg::get(cf_, cntp);
  for (int i = 0; i < nuds; ++i) {
    int vid = *(vars_begin + (cf_.rand_positive() % nvars));
    auto nsvp = std::const_pointer_cast<vertexprop_t>(svp);
    nsvp->add_var(cntp, vassign_->at(vid));
    // TODO: +all dependent
  }
}

void split_tree_t::assign_vars_to(shared_vp_t svp) {
  if (svp->cat() == category_t::LOOP) {
    // we have very special case for loops
    return;
  }

  if (svp->allow_defs())
    add_vars(int(CN::DEFS), svp);

  if (svp->allow_uses())
    add_vars(int(CN::USES), svp);
}

void split_tree_deleter_t::operator()(split_tree_t *pst) { delete pst; }

} // namespace cn