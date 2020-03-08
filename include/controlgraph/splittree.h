//------------------------------------------------------------------------------
//
// Splittree: special labeled tree for internal controlgraph usage
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "controltypes.h"

namespace va {
class varassign_t;
}

namespace cn {

class controlgraph_t;

using itpos_t = std::list<vertex_t>::iterator;

class split_tree_t {
  const controlgraph_t &parent_;

  // copy of config to possibly shake params individually
  cfg::config cf_;

  // varstorage for given split tree (shared with parent)
  std::shared_ptr<va::varassign_t> vassign_;

  // number of function, this split tree is about in parent
  int nfunc_;

  // adjacency lists for vertices
  // we have special illegal vertex 0, adj list of which is
  // all top-level vertices
  static constexpr int PSEUDO_VERTEX = 0;

  std::vector<std::list<vertex_t>> adj_;
  std::unordered_map<vertex_t, vertex_t> parent_of_;
  std::unordered_map<vertex_t, shared_vp_t> desc_of_;
  std::unordered_set<vertex_t> bbs_;

public:
  split_tree_t(const controlgraph_t &p, const cfg::config &cf,
               std::shared_ptr<va::varassign_t> va, int nfunc);

  void process(vcit start, vcit fin);

  // toplevel iterator
  vertex_iter_t begin() const { return adj_[PSEUDO_VERTEX].begin(); }
  vertex_iter_t end() const { return adj_[PSEUDO_VERTEX].end(); }

  // childs iterator
  vertex_iter_t begin_childs(vertex_t parent) const {
    return adj_[parent].begin();
  }

  vertex_iter_t end_childs(vertex_t parent) const { return adj_[parent].end(); }

  shared_vp_t from_vertex(vertex_t v) const;

  std::string varname(va::variable_t v) const;

  // tree-like print of controlgraph
  void dump(std::ostream &os) const;

  // split helpers
private:
  itpos_t add_block(itpos_t pos, vertex_t parent);

  template <typename T, typename... Args>
  void turn_block(int nblock, Args &&... args);

  bool have_parent(int bb, category_t pcat) const;
  void add_container(int bb_under_split);
  void add_special(int bb_under_split);
  void do_split(int bb_under_split);
  void add_vars(int cntp, shared_vp_t svp);
  void assign_vars_to(shared_vp_t svp);
};

//------------------------------------------------------------------------------
//
// Splittree: some templated methods
//
//------------------------------------------------------------------------------

// turns anything at nblock into T
template <typename T, typename... Args>
void split_tree_t::turn_block(int nblock, Args &&... args) {
  desc_of_[nblock] = create_vprop<T>(*this, std::forward<Args>(args)...);
  if constexpr (T::cat != category_t::BLOCK) {
    bbs_.erase(nblock);
  } else {
    bbs_.insert(nblock);
  }
}

} // namespace cn
