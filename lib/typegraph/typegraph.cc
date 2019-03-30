//------------------------------------------------------------------------------
//
// Typegraph impl
//
// ctor sequence is:
// 1. initialize scalars
// 2. seed graph (graph is isolated vertices here)
// 3. do splits (graph is tree here)
// 4. probable unification to make tree into DAG
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

#include "typegraph.h"
#include <boost/graph/graphviz.hpp>

namespace tg {

// string category name for dumps
static const char *cat_str(category_t c) {
  switch (c) {
  case category_t::SCALAR:
    return "T";
  case category_t::STRUCT:
    return "S";
  case category_t::ARRAY:
    return "A";
  case category_t::POINTER:
    return "P";
  default:
    throw std::runtime_error("Unknown category");
  }
}

// label for dot dump of typestorage
std::string vertexprop_t::get_name() const {
  std::ostringstream s;
  s << cat_str(cat) << id;
  return s.str();
}

// dump vertex on ostream
std::ostream &operator<<(std::ostream &os, vertexprop_t v) {
  os << v.get_name() << std::endl;
  return os;
}

//------------------------------------------------------------------------------
//
// Typegraph public interface
//
//------------------------------------------------------------------------------

constexpr int MAX_SPLIT_ATTEMPTS = 10;

// ctor is the only modifying function in typegraph
// see construction sequence description in head comment
typegraph_t::typegraph_t(cfg::config &&cf) : config_(std::move(cf)) {
  if (!config_.quiet())
    std::cout << "Creating typegraph" << std::endl;

  init_scalars();

  // seed graph to be isolated scalar nodes
  int nseeds = cfg::get(config_, TG::SEEDS);
  for (int i = 0; i < nseeds; ++i)
    create_scalar();

  // split graph to forest
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

  // unify to DAG
  // TODO: later

  // add pointer backedges
  // TODO: later

  // create bitfields
  for (auto [vi, vi_end] = boost::vertices(graph_); vi != vi_end; ++vi) {
    if (graph_[*vi].cat != category_t::STRUCT)
      continue;

    struct_t &st = std::get<struct_t>(graph_[*vi].type);

    int bfidx = 0;
    for (auto [ei, ei_end] = boost::out_edges(*vi, graph_); ei != ei_end;
         ++ei, ++bfidx) {
      vertex_t succ = boost::source(*ei, graph_);
      if ((graph_[succ].cat == category_t::SCALAR) &&
          (config_.rand_positive() % 100) < cfg::get(config_, TG::BFPROB)) {
        st.bitfields_[bfidx] = cfg::get(config_, TG::BFSIZE);
      }
    }
  }
}

// dump as dot file
void typegraph_t::dump(std::ostream &os) {
  boost::dynamic_properties dp;
  auto bundle = boost::get(boost::vertex_bundle, graph_);
  dp.property("node_id", boost::get(boost::vertex_index, graph_));
  dp.property("label", boost::make_transform_value_property_map(
                           std::mem_fn(&vertexprop_t::get_name), bundle));
  boost::write_graphviz_dp(os, graph_, dp);
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
  scalars_.emplace_back("unsigned long long", 64, false, false);
  scalars_.emplace_back("long long", 64, false, true);

  // long types may make results unreproducible
  if (cfg::get(config_, TG::LONGT)) {
    scalars_.emplace_back("unsigned long", 32, false, false);
    scalars_.emplace_back("long", 32, false, true);
  }

  // fp types requires special option
  if (cfg::get(config_, TG::FPT)) {
    scalars_.emplace_back("float", 32, true, false);
    scalars_.emplace_back("double", 64, true, false);
  }
}

// create exact scalar
vertex_t typegraph_t::create_scalar() {
  auto sv = boost::add_vertex(graph_);
  leafs_.insert(sv);
  int scid = config_.rand_positive() % scalars_.size();

  graph_[sv].id = sv;
  graph_[sv].cat = category_t::SCALAR;
  graph_[sv].type = scalar_t{scid};
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

// perform split, return positive value on success, 0 on failure
// see split sequence in head comment
int typegraph_t::do_split() {
  auto it = leafs_.begin();
  int n = config_.rand_positive() % leafs_.size();
  std::advance(it, n);
  auto vdesc = *it;
  assert(graph_[vdesc].cat = category_t::SCALAR);

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
    newcont = array_t{nitems};
    break;
  }
  case TGC_STRUCT:
    newcont = struct_t{};
    break;
  default:
    throw(std::runtime_error("Unknown container"));
  }

  // technical split
  int res = split_at(vdesc, newcont);

  // remove if succ
  if (res > 0)
    leafs_.erase(it);

  return res;
}

// split consists of some logic for each container
// like adding new childs, new siblings, etc
// all this encapsulated in this function
int typegraph_t::split_at(vertex_t vdesc, common_t cont) {
  std::visit(
      [this, vdesc, cont](auto &&arg) mutable {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, array_t>) {
          graph_[vdesc].cat = category_t::ARRAY;
          graph_[vdesc].type = cont;
          create_scalar_at(vdesc);
        } else if constexpr (std::is_same_v<T, struct_t>) {
          int nchilds = cfg::get(config_, TG::NFIELDS);
          graph_[vdesc].cat = category_t::STRUCT;
          graph_[vdesc].type = cont;
          for (int i = 0; i < nchilds; ++i)
            create_scalar_at(vdesc);

          // by default all zeros, to be assigned later
          std::get<struct_t>(graph_[vdesc].type).bitfields_.resize(nchilds);
        } else
          throw std::runtime_error("Only structs and arrays are welcome");
      },
      cont);

  return 1;
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