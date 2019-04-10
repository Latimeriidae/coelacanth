//------------------------------------------------------------------------------
//
// Typegraph impl
//
// ctor sequence is:
// 1. initialize scalars
// 2. seed graph (graph is isolated vertices here)
// 3. do splits (graph is tree here)
// 4. unification to make tree into DAG
// 5. create pointers to make DAG into general graph
// 6. assign bitfields
//
// split sequence is:
// 1. peek any leaf node
// 2. generate container basing on probability distributions
// 3. check array/structure constraints to avoid too much nested things
// 4. randomize container details
// 5. do split technical work: assign type, create childs, etc
// 6. remove this one from leafs set if succ
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>
#include <queue>

#include "typegraph.h"
#include <boost/graph/graphviz.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>

namespace ublas = boost::numeric::ublas;

namespace tg {

const char *pseudos[] = {"T", "S", "A", "P"};
constexpr int npseudos = (sizeof(pseudos) / sizeof(const char *));

static_assert(static_cast<int>(category_t::CATMAX) == npseudos);

// short name for callgraph dumps
std::string vertexprop_t::get_short_name() const {
  std::ostringstream s;
  int catidx = static_cast<int>(cat);
  assert(catidx < npseudos);
  s << pseudos[catidx] << id;
  return s.str();
}

// label for dot dump of typestorage
std::string vertexprop_t::get_name() const {
  std::ostringstream s;
  switch (cat) {
  case category_t::SCALAR: {
    const scalar_desc_t *sc = std::get<scalar_t>(type).sdesc;
    s << get_short_name() << " = " << sc->name;
    break;
  }
  case category_t::STRUCT:
    s << get_short_name();
    break;
  case category_t::ARRAY: {
    const auto &ar = std::get<array_t>(type);
    s << get_short_name() << " [" << ar.nitems << "]";
    break;
  }
  case category_t::POINTER:
    s << get_short_name();
    break;
  default:
    throw std::runtime_error("Unknown category");
  }

  return s.str();
}

// dump vertex on ostream
std::ostream &operator<<(std::ostream &os, vertexprop_t v) {
  os << v.get_name() << std::endl;
  return os;
}

vertexprop_t vertex_from(const typegraph_t *pt, vertex_t v) {
  return pt->vertex_from(v);
}

vertex_t dest_from(const typegraph_t *pt, edge_t e) { return pt->dest_from(e); }

//------------------------------------------------------------------------------
//
// Typegraph public interface
//
//------------------------------------------------------------------------------

constexpr int MAX_SPLIT_ATTEMPTS = 10;

// ctor is the only public modifying function in typegraph
// see construction sequence description in head comment
typegraph_t::typegraph_t(cfg::config &&cf) : config_(std::move(cf)) {
  if (!config_.quiet())
    std::cout << "Creating typegraph" << std::endl;

  // initialize scalar classes
  init_scalars();

  // seed graph to be isolated scalar nodes
  int nseeds = cfg::get(config_, TG::SEEDS);
  for (int i = 0; i < nseeds; ++i)
    create_scalar();

  // split graph to forest
  perform_splits();

  // unify to DAG
  unify_subscalars(struct_vs_);
  unify_subscalars(array_vs_);

  // add pointer backedges
  for (auto v : pointer_vs_)
    process_pointer(v);

  // create structure bitfields
  create_bitfields();

  // choose index-like and perm-like types
  // create if none
  choose_perms_idxs();
}

vertex_iter_t typegraph_t::begin() const {
  auto [vi, vi_end] = boost::vertices(graph_);
  return vi;
}

vertex_iter_t typegraph_t::end() const {
  auto [vi, vi_end] = boost::vertices(graph_);
  return vi_end;
}

ct_iterator_t typegraph_t::begin_types() const {
  auto [vi, vi_end] = boost::vertices(graph_);
  return ct_iterator_t(this, vi);
}

ct_iterator_t typegraph_t::end_types() const {
  auto [vi, vi_end] = boost::vertices(graph_);
  return ct_iterator_t(this, vi_end);
}

child_iterator_t typegraph_t::begin_childs(vertex_t v) const {
  auto [ei, ei_end] = boost::out_edges(v, graph_);
  return child_iterator_t(this, ei);
}

child_iterator_t typegraph_t::end_childs(vertex_t v) const {
  auto [ei, ei_end] = boost::out_edges(v, graph_);
  return child_iterator_t(this, ei_end);
}

void typegraph_t::dump(std::ostream &os) const {
  boost::dynamic_properties dp;
  auto bundle = boost::get(boost::vertex_bundle, graph_);
  dp.property("node_id", boost::get(boost::vertex_index, graph_));
  dp.property("label", boost::make_transform_value_property_map(
                           std::mem_fn(&vertexprop_t::get_name), bundle));
  boost::write_graphviz_dp(os, graph_, dp);
}

//------------------------------------------------------------------------------
//
// Random getters public interface
//
//------------------------------------------------------------------------------

vertexprop_t typegraph_t::get_random_type() const {
  cfg::config_rng cfrng(config_);
  vertex_t v = boost::random_vertex(graph_, cfrng);
  return graph_[v];
}

vertexprop_t typegraph_t::get_random_index_type() const {
  assert(idx_vs_.size() > 0);
  int idx = config_.rand_positive() % idx_vs_.size();
  auto it = idx_vs_.begin();
  std::advance(it, idx);
  vertex_t v = *it;
  return graph_[v];
}

vertexprop_t typegraph_t::get_random_perm_type(int nelems) const {
  assert(nelems > 0);
  assert(perm_vs_.size() >= size_t(nelems));
  int idx = config_.rand_positive() % perm_vs_[nelems - 1].size();
  vertex_t v = perm_vs_[nelems - 1][idx];
  return graph_[v];
}

//------------------------------------------------------------------------------
//
// Random getters public interface
//
//------------------------------------------------------------------------------

vertexprop_t typegraph_t::get_pointee(vertex_t v) const {
  auto ci = begin_childs(v);
  assert(ci != end_childs(v));
  auto pointee_type = graph_[(*ci).first];
  ++ci;
  assert(ci == end_childs(v));
  return pointee_type;
}

//------------------------------------------------------------------------------
//
// Typegraph construction helpers
//
//------------------------------------------------------------------------------

// initialize scalar types to choose from
void typegraph_t::init_scalars() {
  scalars_.emplace_back("unsigned char", 8, false, false);
  scalars_.emplace_back("signed char", 8, false, true);
  scalars_.emplace_back("unsigned short", 16, false, false);
  scalars_.emplace_back("short", 16, false, true);
  scalars_.emplace_back("unsigned", 32, false, false);
  scalars_.emplace_back("int", 32, false, true);
  scalars_.emplace_back("unsigned long", 32, false, false);
  scalars_.emplace_back("long", 32, false, true);
  scalars_.emplace_back("unsigned long long", 64, false, false);
  scalars_.emplace_back("long long", 64, false, true);
  scalars_.emplace_back("float", 32, true, true);
  scalars_.emplace_back("double", 64, true, true);

  // we shall have probability function size equal to this initialization
  size_t psize = cfg::prob_size(config_, TG::TYPEPROB);
  if (psize != scalars_.size()) {
    std::ostringstream s;
    s << "There are " << scalars_.size() << " scalar types but only " << psize
      << " entries in discrete probability function";
    throw std::runtime_error(s.str());
  }
}

// create exact scalar
vertex_t typegraph_t::create_scalar() {
  auto sv = boost::add_vertex(graph_);
  int nscal = cfg::get(config_, TG::SCALTYPE);

  switch (nscal) {
  case TGS_POINTER:
    graph_[sv] = create_vprop<pointer_t>(sv);
    pointer_vs_.insert(sv);
    break;
  case TGS_SCALAR: {
    int scid = cfg::get(config_, TG::TYPEPROB);
    graph_[sv] = create_vprop<scalar_t>(sv, &scalars_[scid]);
    leaf_vs_.insert(sv);
    break;
  }
  default:
    throw std::runtime_error("Unknown scalar");
  }

  return sv;
}

// create scalar child
void typegraph_t::create_scalar_at(vertex_t parent) {
  auto sv = create_scalar();
  boost::add_edge(parent, sv, graph_);
}

// get predecessor in tree or null
std::optional<vertex_t> typegraph_t::get_pred(vertex_t v) {
  auto [ei, ei_end] = boost::in_edges(v, graph_);
  if (ei == ei_end)
    return std::nullopt;
  assert(ei_end - ei == 1 && "typegraph is tree at this point");
  vertex_t pred = boost::source(*ei, graph_);
  return pred;
}

// split graph to forest
void typegraph_t::perform_splits() {
  int nsplits = cfg::get(config_, TG::SPLITS);
  for (int i = 0; i < nsplits; ++i) {
    int sres = 0, sres_watchdog = 0;
    while (!sres) {
      sres_watchdog += 1;
      if (sres_watchdog > MAX_SPLIT_ATTEMPTS) {
        std::cerr << "Typegraph warning: to many split attempts in vain"
                  << std::endl;
        break;
      }
      sres = do_split();
    }
  }
}

// perform split, return positive value on success, 0 on failure
// see split sequence in head comment
int typegraph_t::do_split() {
  int n = config_.rand_positive() % leaf_vs_.size();
  auto it = leaf_vs_.begin();
  std::advance(it, n);
  auto vdesc = *it;
  assert(graph_[vdesc].cat == category_t::SCALAR);

  // generate container
  int ncont = cfg::get(config_, TG::CONTTYPE);

  // check constraints
  int narrsup = 0, nstructsup = 0;
  std::optional<vertex_t> p = vdesc;
  for (;;) {
    p = get_pred(p.value());
    if (!p.has_value())
      break;
    if (graph_[p.value()].cat == category_t::ARRAY)
      narrsup += 1;
    if (graph_[p.value()].cat == category_t::STRUCT)
      nstructsup += 1;
  }

  if (narrsup >= cfg::get(config_, TG::MAXARRPREDS))
    return 0;
  if (nstructsup >= cfg::get(config_, TG::MAXSTRUCTPREDS))
    return 0;
  if ((narrsup + nstructsup) >= cfg::get(config_, TG::MAXPREDS))
    return 0;

  // randomize details
  common_t newcont;
  switch (ncont) {
  case TGC_ARRAY: {
    int nitems = cfg::get(config_, TG::ARRSIZE);
    graph_[vdesc] = create_vprop<array_t>(vdesc, nitems);
    array_vs_.insert(vdesc);
    break;
  }
  case TGC_STRUCT:
    graph_[vdesc] = create_vprop<struct_t>(vdesc);
    struct_vs_.insert(vdesc);
    break;
  default:
    throw std::runtime_error("Unknown container");
  }

  // technical split
  int res = split_at(vdesc);

  // remove if succ
  if (res > 0)
    leaf_vs_.erase(it);

  return res;
}

// split consists of some logic for each container
// like adding new childs, new siblings, etc
// all this encapsulated in this function
int typegraph_t::split_at(vertex_t vdesc) {
  switch (graph_[vdesc].cat) {
  case category_t::ARRAY:
    create_scalar_at(vdesc);
    break;
  case category_t::STRUCT: {
    int nchilds = cfg::get(config_, TG::NFIELDS);
    for (int i = 0; i < nchilds; ++i)
      create_scalar_at(vdesc);
    break;
  }
  default:
    throw std::runtime_error("Only structs and arrays are welcome");
  }

  // +1 more top level type for each split
  if (cfg::get(config_, TG::MORESCALARS) != 0) {
    vertex_t newsc = create_scalar();
    leaf_vs_.insert(newsc);
  }

  return 1;
}

void typegraph_t::unify_subscalars(std::set<vertex_t> &vsset) {
  // first unification: similar cells in structures
  ublas::compressed_matrix<vertex_t> unif(ntypes(), scalars_.size());
  for (auto v : vsset)
    for (auto [ei, ei_end] = boost::out_edges(v, graph_); ei != ei_end; ++ei) {
      vertex_t succ = boost::target(*ei, graph_);
      if (graph_[succ].cat != category_t::SCALAR)
        continue;
      auto &&sc = std::get<scalar_t>(graph_[succ].type);

      // index in scalars_ array
      // we may do find or std::distance, but this is much simpler
      int scidx = sc.sdesc - &scalars_[0];
      unif(v, scidx) = succ;
    }

  // now iterate over matrix and unify
  for (auto it1 = unif.begin2(); it1 != unif.end2(); ++it1) {
    auto it2 = it1.begin();
    if (it2 == it1.end() || std::next(it2) == it1.end())
      continue;
    vertex_t unifying_vertex = *it2;
    for (++it2; it2 != it1.end(); ++it2) {
      vertex_t v = *it2;
      vertex_t vpred = it2.index1();
      boost::remove_edge(vpred, v, graph_);
      boost::add_edge(vpred, unifying_vertex, graph_);
    }
  }
}

void typegraph_t::process_pointer(vertex_t v) {
  std::set<vertex_t> pointset;
  std::queue<vertex_t> pointque;

  // add all preds to que
  for (auto [ei, ei_end] = boost::in_edges(v, graph_); ei != ei_end; ++ei) {
    vertex_t parent = boost::source(*ei, graph_);
    if (graph_[parent].cat != category_t::ARRAY)
      pointque.push(parent);
  }

  // process que
  while (!pointque.empty()) {
    vertex_t cur = pointque.front();
    pointque.pop();
    pointset.insert(cur);
    for (auto [esi, esi_end] = boost::out_edges(cur, graph_); esi != esi_end;
         ++esi) {
      vertex_t nxt = boost::target(*esi, graph_);
      if (graph_[nxt].cat != category_t::POINTER)
        pointque.push(nxt);
    }
  }

  // if not lucky, add all scalars and all structs to queue
  if (pointque.empty()) {
    for (auto vl : leaf_vs_)
      pointset.insert(vl);

    for (auto vs : struct_vs_)
      pointset.insert(vs);
  }

  auto it = pointset.begin();
  int n = config_.rand_positive() % pointset.size();
  std::advance(it, n);
  boost::add_edge(v, *it, graph_);
}

void typegraph_t::create_bitfields() {
  for (auto v : struct_vs_) {
    struct_t &st = std::get<struct_t>(graph_[v].type);

    for (auto [ei, ei_end] = boost::out_edges(v, graph_); ei != ei_end; ++ei) {
      vertex_t succ = boost::target(*ei, graph_);
      if ((graph_[succ].cat == category_t::SCALAR) &&
          cfg::get(config_, TG::BFPROB)) {
        auto bfsz = cfg::get(config_, TG::BFSIZE);
        st.bitfields_.push_back(std::make_pair(succ, bfsz));
      }
    }
  }
}

void typegraph_t::choose_perms_idxs() {
  for (auto lf : leaf_vs_) {
    scalar_t &sdt = std::get<scalar_t>(graph_[lf].type);
    if (!sdt.sdesc->is_float)
      idx_vs_.insert(lf);
  }

  auto [szmin, szmax] = cfg::minmax(config_, TG::ARRSIZE);
  perm_vs_.resize(szmax);

  for (auto varr : array_vs_) {
    array_t &adt = std::get<array_t>(graph_[varr].type);

    auto [ei, ei_end] = boost::out_edges(varr, graph_);
    assert(ei + 1 == ei_end);
    vertex_t succ = boost::target(*ei, graph_);
    if (graph_[succ].cat == category_t::SCALAR) {
      scalar_t &sdt = std::get<scalar_t>(graph_[succ].type);
      if (!sdt.sdesc->is_float)
        perm_vs_[adt.nitems - 1].push_back(varr);
    }
  }

  if (idx_vs_.empty()) {
    auto scalit =
        find_if(scalars_.begin(), scalars_.end(),
                [](const scalar_desc_t &sd) { return (sd.name == "int"); });
    if (scalit == scalars_.end())
      throw std::runtime_error(
          "You shall allow int type in order for indexes to work");

    // adding scalar
    auto sv = boost::add_vertex(graph_);
    graph_[sv] = create_vprop<scalar_t>(sv, &*scalit);
    leaf_vs_.insert(sv);
    idx_vs_.insert(sv);
  }

  assert(!idx_vs_.empty());

  // now we need to create all permutations up to max array index if they aren't
  // exist
  auto sv = *idx_vs_.begin();
  for (int cur = szmin; cur != szmax; ++cur)
    if (perm_vs_[cur - 1].empty()) {
      auto sva = boost::add_vertex(graph_);
      graph_[sva] = create_vprop<array_t>(sva, cur);
      boost::add_edge(sva, sv, graph_);
      perm_vs_[cur - 1].push_back(sva);
      array_vs_.insert(sva);
    }
}

} // namespace tg

//------------------------------------------------------------------------------
//
// Task system support
//
//------------------------------------------------------------------------------

std::shared_ptr<tg::typegraph_t> typegraph_create(int seed,
                                                  const cfg::config &cf) {
  try {
    cfg::config newcf(seed, cf.quiet(), cf.cbegin(), cf.cend());
    auto tg = std::make_shared<tg::typegraph_t>(std::move(newcf));
    return tg;
  } catch (std::runtime_error &e) {
    std::cerr << "Typegraph construction problem: " << e.what() << std::endl;
    throw;
  }
}

void typegraph_dump(std::shared_ptr<tg::typegraph_t> tg, std::ostream &os) {
  tg->dump(os);
}