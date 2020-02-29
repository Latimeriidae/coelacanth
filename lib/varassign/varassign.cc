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

#include <queue>

#include "../dbgstream.h"
#include "callgraph/callgraph.h"
#include "typegraph/typegraph.h"
#include "varassign.h"

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

  // create global variables
  int nvars = cfg::get(config_, VA::NGLOBALS);
  for (int vidx = 0; vidx != nvars; ++vidx) {
    auto vpt = tgraph_->get_random_type();
    int vid = create_var(vpt.id);
    globals_.insert(vid);
  }

  // create function variable subsets
  fvars_.resize(cgraph_->nfuncs());
  for (auto fit = cgraph_->begin(); fit != cgraph_->end(); ++fit)
    create_function_vars(*fit);
}

std::string varassign_t::get_name(int vid, int funcid) const {
  std::ostringstream os;
  if (is_global(vid)) {
    os << "g";
  } else if ((funcid != -1) && (fvars_[funcid].is_perm(vid))) {
    os << "p";
  } else if ((funcid != -1) && (fvars_[funcid].is_index(vid))) {
    os << "i";
  } else {
    os << "v";
  }

  os << vid;

  return os.str();
}

void varassign_t::dump(std::ostream &os) const {
  os << "Globals\n";
  for (auto v : globals_) {
    auto vpt = tgraph_->vertex_from(vars_[v].type_id);
    os << vpt.get_short_name() << " " << get_name(v, -1) << std::endl;
  }

  for (int f = 0, fe = fvars_.size(); f != fe; ++f) {
    os << "Function #" << f << "\n";
    for (auto v : fvars_[f].vars_) {
      auto vpt = tgraph_->vertex_from(vars_[v].type_id);
      os << vpt.get_short_name() << " " << get_name(v, f) << std::endl;
    }
  }
}

//------------------------------------------------------------------------------
//
// Construction helpers
//
//------------------------------------------------------------------------------

// create single variable and add to storage
int varassign_t::create_var(int tid) {
  int vidx = vars_.size();
  vars_.emplace_back(vidx, tid);
  return vidx;
}

void varassign_t::create_pointee(int vid, int tid, func_vars &fv) {
  int pointee_vid = create_var(tgraph_->get_pointee(tid).id);
  fv.pointees_[vid][tid] = pointee_vid;
  fv.vars_.push_back(pointee_vid);
}

void varassign_t::process_var(int vid, int funcid) {
  int tid = vars_[vid].type_id;
  tg::vertexprop_t vpt = tgraph_->vertex_from(tid);

  auto &fv = fvars_[funcid];

  // create pointees for pointers
  if (vpt.is_pointer()) {
    create_pointee(vid, tid, fv);
  }

  // create permutators for arrays
  // TODO: we, theoretically, can permute arrays inside structures...
  //       those "subpermutators" arent now supported
  if (vpt.is_array()) {
    int nitems = std::get<tg::array_t>(vpt.type).nitems;
    while (cfg::get(config_, VA::USEPERM)) {
      int perm_vid = create_var(tgraph_->get_random_perm_type(nitems).id);
      fv.perms_.insert(perm_vid);
      fv.permutators_[vid].push_back(perm_vid);
      fv.vars_.push_back(perm_vid);

      if (cfg::get(config_, VA::MAXPERM) == int(fv.permutators_.size()))
        break;
    }
  }

  // create indexes for accessors
  std::queue<tg::vertexprop_t> chlds;
  if (vpt.is_complex())
    chlds.push(vpt);

  while (!chlds.empty()) {
    auto cpt = chlds.front();
    chlds.pop();

    if (cpt.is_array()) {
      int index_vid = create_var(tgraph_->get_random_index_type().id);
      fv.indexes_.insert(index_vid);
      fv.accidxs_[vid].push_back(index_vid);
      fv.vars_.push_back(index_vid);
    }

    for (auto cit = tgraph_->begin_childs(cpt.id);
         cit != tgraph_->end_childs(cpt.id); ++cit) {
      auto npt = tgraph_->vertex_from((*cit).first);
      if (npt.is_complex())
        chlds.push(npt);
      if (npt.is_pointer())
        create_pointee(vid, npt.id, fv);
    }
  }
}

void varassign_t::create_function_vars(int funcid) {
  assert(funcid < cgraph_->nfuncs());
  cfg::config_rng cfrng(config_);
  auto &fv = fvars_[funcid];

  // add free indexes
  int nidx = cfg::get(config_, VA::NIDX);
  for (int vidx = 0; vidx != nidx; ++vidx) {
    int iid = create_var(tgraph_->get_random_index_type().id);
    fv.register_index(iid);
  }

  // choose from all globals, those, that conform to metastructure
  for (auto gid : globals_) {
    if (!cgraph_->accept_type(funcid, gid))
      continue;
    fv.vars_.push_back(gid);
    process_var(gid, funcid);
  }

  // add local variables
  int vidx = 0;
  int nvars = cfg::get(config_, MS::NVARS);
  int nvatts = cfg::get(config_, VA::NVATTS);
  while (vidx < nvars) {
    auto vpt = tgraph_->get_random_type();
    if (cgraph_->accept_type(funcid, vpt.id)) {
      int vid = create_var(vpt.id);
      fv.vars_.push_back(vid);
      process_var(vid, funcid);
      vidx += 1;
    }
    if (nvatts == 0)
      break;
    nvatts -= 1;
  }

  // add argument variables
  auto vpt = cgraph_->vertex_from(funcid);

  for (auto tid : vpt.argtypes) {
    int vid = create_var(tid);
    fv.args_.insert(vid);
    fv.vars_.push_back(vid);
    process_var(vid, funcid);
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
    cfg::config newcf(seed, cf.quiet(), cf.dumps(), cf.cbegin(), cf.cend());
    return std::make_shared<va::varassign_t>(std::move(newcf), sptg, spcg);
  } catch (std::runtime_error &e) {
    std::cerr << "Varassign construction problem: " << e.what() << std::endl;
    throw;
  }
}

void varassign_dump(std::shared_ptr<va::varassign_t> pv, std::ostream &os) {
  pv->dump(os);
}
