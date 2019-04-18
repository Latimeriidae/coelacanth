//------------------------------------------------------------------------------
//
// Controltypes: all block & container subtypes for controlgraph
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
#include "varassign/variable.h"

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
  ACCESS,
  BREAK,
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
  call_t(call_type_t t, int n) : type(t), nfunc(n) {}
};

struct loop_t {
  static constexpr category_t cat = category_t::LOOP;
  int start, stop, step;
  loop_t(int from, int to, int s) : start(from), stop(to), step(s) {}
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

struct access_t {
  static constexpr category_t cat = category_t::ACCESS;
};

enum class break_type_t { CONTINUE, BREAK, RETURN };

struct break_t {
  static constexpr category_t cat = category_t::BREAK;
  break_type_t btp;
  break_t(break_type_t b) : btp(b) {}
};

using common_t = std::variant<block_t, call_t, loop_t, if_t, switch_t, region_t,
                              branching_t, access_t, break_t>;

class split_tree_t;

struct split_tree_deleter_t {
  void operator()(split_tree_t *);
};

using stt = std::unique_ptr<split_tree_t, split_tree_deleter_t>;

// vertexprop is rather large, so real operating type is shared-pointer to it
// see shared_vp_t below
class vertexprop_t {
  const split_tree_t &parent_;
  vertex_t id_ = ILLEGAL_VERTEX;
  category_t cat_ = category_t::ILLEGAL;
  common_t type_;

  std::vector<va::variable_t> defs_;
  std::vector<va::variable_t> uses_;

public:
  using vait = typename std::vector<va::variable_t>::const_iterator;

  vertexprop_t() = default;
  explicit vertexprop_t(const split_tree_t &p, vertex_t i, category_t c,
                        common_t t)
      : parent_(p), id_(i), cat_(c), type_(t) {}

  vertexprop_t(const vertexprop_t &) = default;
  vertexprop_t &operator=(const vertexprop_t &) = default;

  category_t cat() const { return cat_; }
  common_t type() const { return type_; }
  bool is_block() const { return cat_ == category_t::BLOCK; }

  // pseudo-nodes like if may have no uses
  // every branching have (last one may ignore though)
  bool allow_uses() const {
    return cat_ != category_t::IF && cat_ != category_t::SWITCH &&
           cat_ != category_t::REGION;
  }

  bool allow_defs() const {
    return cat_ == category_t::BLOCK || cat_ == category_t::CALL;
  }
  bool is_branching() const {
    return cat_ == category_t::IF || cat_ == category_t::SWITCH ||
           cat_ == category_t::REGION;
  }

  vait defs_begin() const { return defs_.cbegin(); }
  vait defs_end() const { return defs_.cend(); }
  vait uses_begin() const { return uses_.cbegin(); }
  vait uses_end() const { return uses_.cend(); }
  std::string varname(va::variable_t v) const;

  void add_var(int cntp, va::variable_t v) {
    if (cntp == int(CN::DEFS))
      defs_.push_back(v);
    else
      uses_.push_back(v);
  }
};

using shared_vp_t = std::shared_ptr<const vertexprop_t>;
using vct = std::vector<shared_vp_t>;
using vcit = typename vct::iterator;

std::ostream &operator<<(std::ostream &, const vertexprop_t);

template <typename T, typename... Ts>
shared_vp_t create_vprop(const split_tree_t &p, vertex_t id, Ts &&... args) {
  return std::make_shared<const vertexprop_t>(p, id, T::cat,
                                              T(std::forward<Ts>(args)...));
}

using vertex_iter_t = typename std::list<vertex_t>::const_iterator;

} // namespace cn