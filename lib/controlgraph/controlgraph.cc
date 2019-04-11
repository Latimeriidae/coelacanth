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

#include "controlgraph.h"
#include "../dbgstream.h"
#include "callgraph/callgraph.h"
#include "typegraph/typegraph.h"
#include "varassign/varassign.h"

namespace cn {

//------------------------------------------------------------------------------
//
// Controlgraph public interface
//
//------------------------------------------------------------------------------

controlgraph_t::controlgraph_t(cfg::config &&cf,
                               std::shared_ptr<tg::typegraph_t> tgraph,
                               std::shared_ptr<cg::callgraph_t> cgraph,
                               std::shared_ptr<va::varassign_t> vassign)
    : config_(std::move(cf)), tgraph_(tgraph), cgraph_(cgraph),
      vassign_(vassign) {
  if (!config_.quiet())
    dbgs() << "Creating controlgraph\n";
}

void controlgraph_t::dump(std::ostream &) {}

} // namespace cn

//------------------------------------------------------------------------------
//
// Task system support
//
//------------------------------------------------------------------------------

std::shared_ptr<cn::controlgraph_t>
controlgraph_create(int seed, const cfg::config &cf,
                    std::shared_ptr<tg::typegraph_t> sptg,
                    std::shared_ptr<cg::callgraph_t> spcg,
                    std::shared_ptr<va::varassign_t> spva) {
  try {
    cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
    return std::make_shared<cn::controlgraph_t>(std::move(newcf), sptg, spcg,
                                                spva);
  } catch (std::runtime_error &e) {
    std::cerr << "Controlgraph construction problem: " << e.what() << std::endl;
    throw;
  }
}

void controlgraph_dump(std::shared_ptr<cn::controlgraph_t> pc,
                       std::ostream &os) {
  pc->dump(os);
}
