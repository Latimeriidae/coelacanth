//------------------------------------------------------------------------------
//
// Type analysis: type direct reachability for storage
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <boost/numeric/ublas/matrix_sparse.hpp>

#include "typegraph.h"

namespace ublas = boost::numeric::ublas;

namespace tg {

// like enum class, but I am worrying about keeping enum class in matrix
namespace compat_t {
constexpr char NONE = 0;
constexpr char DIRECT = 1;
constexpr char INDIRECT = 2;
} // namespace compat_t

class type_analysis_t {
  ublas::compressed_matrix<char> acc_;

public:
  type_analysis_t(const typegraph_t &tg) : acc_(tg.ntypes(), tg.ntypes()) {
    for (auto it = tg.begin(); it != tg.end(); ++it) {
      auto vtxid = *it;
      auto prop = tg.get_type(vtxid);
      assert(prop.id == vtxid);

      // self is always accessible
      acc_(vtxid, vtxid) = compat_t::DIRECT;

      // put all childs to be accessible
      for (auto ci = tg.begin_childs(vtxid); ci != tg.end_childs(vtxid); ++ci)
        acc_(vtxid, (*ci).first) = (prop.cat == category_t::POINTER)
                                       ? compat_t::INDIRECT
                                       : compat_t::DIRECT;

      // exclude bitfields if any
      if (prop.cat == category_t::STRUCT) {
        auto st = get<struct_t>(prop.type);
        for (auto bf : st.bitfields_)
          acc_(vtxid, bf.first) = compat_t::NONE;
      }
    }

    // transitive closure of matrix
    for (int k = 0; k < tg.ntypes(); ++k)
      for (int i = 0; i < tg.ntypes(); ++i)
        for (int j = 0; j < tg.ntypes(); ++j) {
          // If vertex k is on a path from i to j,
          // then make sure that the value of reach[i][j] is 1
          // reach[i][j] = reach[i][j] || (reach[i][k] && reach[k][j]);
          if (acc_(i, j) != compat_t::NONE)
            continue;

          if ((acc_(i, k) == compat_t::NONE) || (acc_(k, j) == compat_t::NONE))
            continue;

          if ((acc_(i, k) == compat_t::INDIRECT) ||
              (acc_(k, j) == compat_t::INDIRECT))
            acc_(i, j) = compat_t::INDIRECT;
          else
            acc_(i, j) = compat_t::DIRECT;
        }
  }

  bool has_access(vertex_t from, vertex_t to) const {
    return acc_(from, to) != compat_t::NONE;
  }
  bool has_direct_access(vertex_t from, vertex_t to) const {
    return acc_(from, to) == compat_t::DIRECT;
  }
};

} // namespace tg