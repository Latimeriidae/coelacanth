//------------------------------------------------------------------------------
//
// Varassign: basic variable assignments for callgraph functions
//
// General idea of varassign: mappings and sets
//
// 1. Sets
// 1.1 special set of global vars
// 1.2 special set of permutation vars
// 1.3 special set of index vars
//
// 2. Function-independent mappings
// 2.1 vars to vars (pointees)
// 2.2 vars to vars (acc idxs)
// 2.3 vars to vars (permutators)
//
// 3. Function-dependent subsets
// 3.1 function-used variables
// 3.2 function-used argument variables
//
// Each variable have:
// * global id
// * type id
// * special meaning (like "permutation" or "index")
// * optionally: function-specific name
//
// Example: in function 5, variable 15 have type A5 (array of int)
//          special meaning PERM and name p3 inside function
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "config/configs.h"
#include "variable.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tg {
class typegraph_t;
}

namespace cg {
class callgraph_t;
}

namespace va {

using acc_cont_t = std::vector<int>;
using accit_t = typename acc_cont_t::const_iterator;
using perm_cont_t = std::vector<int>;
using permit_t = typename perm_cont_t::const_iterator;

class varassign_t final {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  std::shared_ptr<cg::callgraph_t> cgraph_;

  // storage for variables vx
  std::vector<variable_t> vars_;

  // global variables gx
  std::unordered_set<int> globals_;

  struct func_vars {
    // function vars
    std::vector<int> vars_;

    // permutators px
    std::unordered_set<int> perms_;

    // indexes ix
    std::unordered_set<int> indexes_;

    // arguments xx
    std::unordered_set<int> args_;

    // pointees: x -> y -> z
    // If v[x] have pointer subtypes, they point to some v[z]
    // Namely, pts[x] gives map of subtypes to pointee variables
    std::unordered_map<int, std::unordered_map<int, int>> pointees_;

    // accessor idxs
    // array or struct with array members vx may have accessors iy, iz, ....
    std::unordered_map<int, acc_cont_t> accidxs_;

    // permutator idxs
    // array vx[iz] may have permutator py[iz] (also array) like this:
    // vx[py[iz]] we may have a lot of stacked permutators vx[pa[pb[pc[iz]]]]
    std::unordered_map<int, perm_cont_t> permutators_;

    void register_index(int iid) {
      indexes_.insert(iid);
      vars_.push_back(iid);
    }

    bool is_perm(int vid) const { return perms_.find(vid) != perms_.end(); }
    bool is_index(int vid) const {
      return indexes_.find(vid) != indexes_.end();
    }
    bool is_argument(int vid) const { return args_.find(vid) != args_.end(); }
  };

  bool is_global(int vid) const { return globals_.find(vid) != globals_.end(); }

  // every function have some variables
  std::vector<func_vars> fvars_;

  // public interface
public:
  explicit varassign_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>,
                       std::shared_ptr<cg::callgraph_t>);

  // iterators for all vars
  // value type is variable_t i.e. variable + type
  auto begin() const { return vars_.cbegin(); }
  auto end() const { return vars_.cend(); }

  variable_t at(int n) const { return vars_[n]; }

  // iterators for specific function vars (useful for controlgraph split)
  // value type is int, i.e. index in all vars
  auto fv_begin(int nfunc) const { return fvars_[nfunc].vars_.cbegin(); }
  auto fv_end(int nfunc) const { return fvars_[nfunc].vars_.cend(); }

  bool have_pointee(int nfunc, int vid, int tid) const {
    auto &ps = fvars_[nfunc].pointees_;
    auto pcit = ps.find(vid);
    if (pcit == ps.end())
      return false;
    return pcit->second.find(tid) != pcit->second.end();
  }

  int pointee(int nfunc, int vid, int tid) const {
    assert(have_pointee(nfunc, vid, tid) &&
           "No pointee for this variable subtype");
    return fvars_[nfunc].pointees_.at(vid).at(tid);
  }

  bool have_accs(int nfunc, int vid) const {
    auto &accs = fvars_[nfunc].accidxs_;
    auto accit = accs.find(vid);
    if (accit == accs.end())
      return false;
    return !accit->second.empty();
  }

  accit_t accs_begin(int nfunc, int vid) const {
    auto accit = fvars_[nfunc].accidxs_.find(vid);
    return accit->second.cbegin();
  }

  accit_t accs_end(int nfunc, int vid) const {
    auto accit = fvars_[nfunc].accidxs_.find(vid);
    return accit->second.cend();
  }

  std::string get_name(int vid, int funcid) const;

  void dump(std::ostream &os) const;

  // helpers
private:
  int create_var(int tid);
  void create_pointee(int vid, int tid, func_vars &fv);
  void process_var(int vid, int funcid);
  void create_function_vars(int fid);
};

} // namespace va