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

  // create variables
  int nvars = cfg::get(config_, VA::NVARS);
  vars_.resize(nvars);
  for (int vidx = 0; vidx != nvars; ++vidx) {
    auto vpt = tgraph_->get_random_type();
    vars_[vidx].id = vidx;
    vars_[vidx].type_id = vpt.id;
  }

  // choosing globals
  int nglobs = cfg::get(config_, VA::NGLOBALS);
  // TODO: this restriction shall be in option processing
  if (nglobs > nvars)
    throw std::runtime_error("We can not have more globals then variables");

  for (int vidx = 0; vidx != nglobs; ++vidx)
    globals_.insert(vidx);

  // add free indexes
  int nidx = cfg::get(config_, VA::NIDX);
  for (int vidx = 0; vidx != nidx; ++vidx) {
    int vsz = vars_.size();
    auto vpt = tgraph_->get_random_index_type();
    vars_.emplace_back(vsz, vpt.id);
    indexes_.insert(vsz);
  }

  // create pointee for any pointer
  for (auto v : vars_) {
    if (!tgraph_->vertex_from(v.type_id).is_pointer())
      continue;
    auto pointee_type = tgraph_->get_pointee(v.type_id);
    int vsz = vars_.size();
    vars_.emplace_back(vsz, pointee_type.id);
    pointees_[v.id] = vsz;
  }

  // create function variable subsets
  for (auto fit = cgraph_->begin(); fit != cgraph_->end(); ++fit) {
  }

  // add function argument variables
  for (auto fit = cgraph_->begin(); fit != cgraph_->end(); ++fit) {
  }

  // create permutators for variables
  for (auto v : vars_) {
    auto vpt = tgraph_->vertex_from(v.type_id);
    if (!vpt.is_array())
      continue;
    int nitems = std::get<tg::array_t>(vpt.type).nitems;
    while (cfg::get(config_, VA::USEPERM)) {
      auto vptp = tgraph_->get_random_perm_type(nitems);
      int vsz = vars_.size();
      vars_.emplace_back(vsz, vptp.id);
      permutators_[v.id].push_back(vsz);
      if (cfg::get(config_, VA::MAXPERM) == int(permutators_.size()))
        break;
    }
  }

  // create indexes for accessors
  for (auto v : vars_) {
    auto vpt = tgraph_->vertex_from(v.type_id);
    if (!vpt.is_complex())
      continue;
  }

  // set pointer indexes as pointee ones
  for (auto v : vars_) {
    if (!tgraph_->vertex_from(v.type_id).is_pointer())
      continue;
  }
}

std::string varassign_t::get_name(int vid) const {
  std::ostringstream os;
  if (is_global(vid)) {
    os << "g";
  } else if (is_perm(vid)) {
    os << "p";
  } else if (is_index(vid)) {
    os << "i";
  } else {
    os << "v";
  }

  os << vid;

  return os.str();
}

void varassign_t::dump(std::ostream &os) const {
  for (auto v : vars_) {
    auto vpt = tgraph_->vertex_from(v.type_id);
    os << vpt.get_short_name() << " " << get_name(v.id) << std::endl;
  }
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
