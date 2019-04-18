//------------------------------------------------------------------------------
//
// Control-flow (top-level, before locations) for given functions
//
// control graph itself is array of split trees, one for each call node
// plus some public interface
//
// split tree details left for implementation
//
// Node types:
//
// 1. Loops
//
// LOOP -> [ BODY ]
//
// it is something like for, do-while, while, etc...
//
// 2. Conditional operands
//
// IF -> BRANCHING -> [ BODY ]
//    -> BRANCHING -> [ BODY ]
//    -> BRANCHING -> [ BODY ]
//
// SWITCH -> BRANCHING -> [ BODY ]
//        -> BRANCHING -> [ BODY ]
//        -> BRANCHING -> [ BODY ]
//
// it is something like if-elseif-else or switch case-case-default
//
// 3. Irreducible regions
//
// REGION -> BRANCHING -> [ BODY ]
//        -> BRANCHING -> [ BODY ]
//        -> BRANCHING -> [ BODY ]
//
// 4. BLOCKs, function CALLs, etc
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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config/configs.h"
#include "controltypes.h"

namespace tg {
class typegraph_t;
}

namespace cg {
class callgraph_t;
}

namespace va {
class varassign_t;
}

namespace cn {

class controlgraph_t final {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  std::shared_ptr<cg::callgraph_t> cgraph_;
  std::shared_ptr<va::varassign_t> vassign_;

  std::vector<stt> strees_;

  // public interface
public:
  explicit controlgraph_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>,
                          std::shared_ptr<cg::callgraph_t>,
                          std::shared_ptr<va::varassign_t>);

  int nfuncs() const;

  // top-level iterator by function number
  vertex_iter_t begin(int nfunc) const;
  vertex_iter_t end(int nfunc) const;

  // childs iterator
  vertex_iter_t begin_childs(int nfunc, vertex_t parent) const;
  vertex_iter_t end_childs(int nfunc, vertex_t parent) const;

  shared_vp_t from_vertex(int nfunc, vertex_t) const;
  std::string varname(int nfunc, int vid) const;

  // get random call target from call graph or -1 if no available
  int random_callee(int nfunc, call_type_t) const;

  void dump(std::ostream &os) const;

  // public but not recommended for unenlightened usage interface
public:
  void add_vars(int cntp, int nfunc, vertexprop_t &vp) const;
  void assign_vars_to(int nfunc, vertexprop_t &vp) const;
};

} // namespace cn
