//------------------------------------------------------------------------------
//
// Typegraph impl
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>

#include "typegraph.h"

namespace tg {
typegraph_t::typegraph_t(cfg::config &&cf) : config_(std::move(cf)) {
  if (!config_.quiet())
    std::cout << "Creating typegraph" << std::endl;
}
} // namespace tg

// task system support
std::shared_ptr<tg::typegraph_t> typegraph_create(int seed,
                                                  const cfg::config &cf) {
  cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
  return std::make_shared<tg::typegraph_t>(std::move(newcf));
}
