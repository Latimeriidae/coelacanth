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

using vertex_t = int;
constexpr vertex_t ILLEGAL_VERTEX = -1;

enum class category_t {
  ILLEGAL = -1,
  BLOCK = 0,
  CALL,
  LOOP,
  IF,
  SWITCH,
  REGION,
  BRANCHING,
  CATMAX
};

struct block_t {
  static constexpr category_t cat = category_t::BLOCK;
};

enum class call_type_t { DIRECT, CONDITIONAL, INDIRECT };

struct call_t {
  static constexpr category_t cat = category_t::CALL;
  call_type_t type;
  int nfunc;
};

struct loop_t {
  static constexpr category_t cat = category_t::LOOP;
  int start, stop, step;
};

struct if_t {
  static constexpr category_t cat = category_t::IF;
};

struct switch_t {
  static constexpr category_t cat = category_t::SWITCH;
};

struct region_t {
  static constexpr category_t cat = category_t::REGION;
};

struct branching_t {
  static constexpr category_t cat = category_t::BRANCHING;
};

using common_t = std::variant<block_t, call_t, loop_t, if_t, switch_t, region_t,
                              branching_t>;

struct vertexprop_t {
  vertex_t id = ILLEGAL_VERTEX;
  category_t cat = category_t::ILLEGAL;
  common_t type;

  vertexprop_t() = default;
  explicit vertexprop_t(vertex_t i, category_t c, common_t t)
      : id(i), cat(c), type(t) {}
};

template <typename T, typename... Ts>
vertexprop_t create_vprop(vertex_t id, Ts &&... args) {
  return vertexprop_t{id, T::cat, T{std::forward<Ts>(args)...}};
}

class split_tree_t;

struct split_tree_deleter_t {
  void operator()(split_tree_t *);
};

using stt = std::unique_ptr<split_tree_t, split_tree_deleter_t>;

using vertex_iter_t = typename std::list<vertex_t>::const_iterator;

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

  int nfuncs() const { return cgraph_->nfuncs(); }

  // top-level iterator by function number
  vertex_iter_t begin(int nfunc) const;
  vertex_iter_t end(int nfunc) const;

  // childs iterator
  vertex_iter_t begin_childs(int nfunc, vertex_t parent) const;
  vertex_iter_t end_childs(int nfunc, vertex_t parent) const;

  vertexprop_t from_vertex(int nfunc, vertex_t) const;

  void dump(std::ostream &os) const;
};

} // namespace cn
