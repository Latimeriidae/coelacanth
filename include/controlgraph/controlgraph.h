//------------------------------------------------------------------------------
//
// Control-flow (top-level, before locations) for given functions
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <memory>

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

class controlgraph_t final {
  cfg::config config_;
  std::shared_ptr<tg::typegraph_t> tgraph_;
  std::shared_ptr<cg::callgraph_t> cgraph_;
  std::shared_ptr<va::varassign_t> vassign_;

  // public interface
public:
  explicit controlgraph_t(cfg::config &&, std::shared_ptr<tg::typegraph_t>,
                          std::shared_ptr<cg::callgraph_t>,
                          std::shared_ptr<va::varassign_t>);

  void dump(std::ostream &os);
};

} // namespace cn
