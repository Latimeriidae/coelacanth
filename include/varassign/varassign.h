//------------------------------------------------------------------------------
//
// Varassign: basic variable assignments for callgraph functions
//
// General idea of varassign: mappings
//
// func to globals
// func to locals
// func to free idxs
// locals + globals to locals (pointees)
// locals + globals to acc idxs
// locals + globals to perms
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

namespace tg {
class typegraph_t;
}

namespace cg {
class callgraph_t;
}

namespace va {

class varassign_t final {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  std::shared_ptr<cg::callgraph_t> cgraph_;

  // public interface
public:
  explicit varassign_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>,
                       std::shared_ptr<cg::callgraph_t>);
};

} // namespace va