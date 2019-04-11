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

struct variable_t {
  int id;
  int type_id;
  variable_t(int i = -1, int ti = -1) : id(i), type_id(ti) {}
};

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

    // pointees: x -> y
    // If v[x] have pointer type, it points to some v[y]
    std::unordered_map<int, int> pointees_;

    // accessor idxs
    // array or struct with array members vx may have accessors iy, iz, ....
    std::unordered_map<int, std::vector<int>> accidxs_;

    // permutator idxs
    // array vx[iz] may have permutator py[iz] (also array) like this:
    // vx[py[iz]] we may have a lot of stacked permutators vx[pa[pb[pc[iz]]]]
    std::unordered_map<int, std::vector<int>> permutators_;

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
  auto begin() const { return vars_.cbegin(); }
  auto end() const { return vars_.cend(); }

  std::string get_name(int vid, int funcid) const;

  void dump(std::ostream &os) const;

  // helpers
private:
  int create_var(int tid);
  void process_var(int vid, int funcid);
  void create_function_vars(int fid);
};

} // namespace va