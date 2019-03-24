//------------------------------------------------------------------------------
//
// High-level abstract interface for call-graph
//
//
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "config/configs.h"

namespace cg {

class callgraph {
  // TODO: private
public:
  callgraph(const cfg::config &);
};

} // namespace cg
