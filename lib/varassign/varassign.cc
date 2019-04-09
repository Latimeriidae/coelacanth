//------------------------------------------------------------------------------
//
// Varassign: basic variable assignments for callgraph functions
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "varassign.h"
#include "../dbgstream.h"
#include "callgraph/callgraph.h"
#include "typegraph/typegraph.h"

namespace va {

//------------------------------------------------------------------------------
//
// Varassign public interface
//
//------------------------------------------------------------------------------

varassign_t::varassign_t(cfg::config &&cf,
                         std::shared_ptr<tg::typegraph_t> tgraph,
                         std::shared_ptr<cg::callgraph_t> cgraph)
    : config_(std::move(cf)), tgraph_(tgraph), cgraph_(cgraph) {
  if (!config_.quiet())
    dbgs() << "Creating varassign\n";
}

} // namespace va

//------------------------------------------------------------------------------
//
// Task system support
//
//------------------------------------------------------------------------------

std::shared_ptr<va::varassign_t>
varassign_create(int seed, const cfg::config &cf,
                 std::shared_ptr<tg::typegraph_t> sptg,
                 std::shared_ptr<cg::callgraph_t> spcg) {
  try {
    cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
    return std::make_shared<va::varassign_t>(std::move(newcf), sptg, spcg);
  } catch (std::runtime_error &e) {
    std::cerr << "Varassign construction problem: " << e.what() << std::endl;
    throw;
  }
}
