//------------------------------------------------------------------------------
//
// Callgraph impl
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>

#include "callgraph.h"

namespace cg {
callgraph_t::callgraph_t(cfg::config &&cf,
                         std::shared_ptr<tg::typegraph_t> tgraph)
    : config_(std::move(cf)), tgraph_(std::move(tgraph)) {
  if (!config_.quiet())
    std::cout << "Creating callgraph" << std::endl;
}
} // namespace cg

// task system support
std::shared_ptr<cg::callgraph_t>
callgraph_create(int seed, const cfg::config &cf,
                 std::shared_ptr<tg::typegraph_t> sptg) {
  cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
  return std::make_shared<cg::callgraph_t>(std::move(newcf), sptg);
}
