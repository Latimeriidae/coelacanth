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
callgraph::callgraph(const cfg::config &) {}
} // namespace cg

// task system support
std::shared_ptr<cg::callgraph> callgraph_create(const cfg::config &cfg) {
  if (!cfg.quiet())
    std::cout << "Creating call graph" << std::endl;
  return std::make_shared<cg::callgraph>(cfg);
}
