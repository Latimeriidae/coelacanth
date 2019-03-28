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
#include <boost/graph/graphviz.hpp>

namespace tg {

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

std::string vertexprop_t::get_name() const {
  std::ostringstream s;
  s << cat_str(cat) << id;
  return s.str();
}

std::ostream &operator<<(std::ostream &os, vertexprop_t v) {
  os << v.get_name() << std::endl;
  return os;
}

//------------------------------------------------------------------------------
//
// Typegraph public interface
//
//------------------------------------------------------------------------------

typegraph_t::typegraph_t(cfg::config &&cf) : config_(std::move(cf)) {
  if (!config_.quiet())
    std::cout << "Creating typegraph" << std::endl;

  // 1. initialize scalars
  init_scalars();

  // 2. seed graph
  int nseeds = cfg::get(config_, TG::SEEDS);

  for (int i = 0; i < nseeds; ++i)
    create_scalar();

  // 3. do splits
  int nsplits = cfg::get(config_, TG::SPLITS);

  for (int i = 0; i < nsplits; ++i)
    do_split();
}

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

vertex_t typegraph_t::create_scalar() {
  auto sv = boost::add_vertex(graph_);
  leafs_.insert(sv);
  int scid = config_.rand_positive() % scalars_.size();

  graph_[sv].id = boost::num_vertices(graph_) - 1;
  graph_[sv].cat = category_t::SCALAR;
  graph_[sv].type = scalar_t{scid};
  return sv;
}

void typegraph_t::create_scalar_at(vertex_t parent) {
  auto sv = create_scalar();
  boost::add_edge(parent, sv, graph_);
}

void typegraph_t::do_split() {
  // 1. peek any leaf node
  auto it = leafs_.begin();
  int n = config_.rand_positive() % leafs_.size();
  std::advance(it, n);
  auto vdesc = *it;

  // 2. removing this one from leafs
  leafs_.erase(it);

  // 3. with some probability generate array or structure
  int ncont = cfg::get(config_, TG::CONTTYPE);
  switch (ncont) {
  case TGC_ARRAY: {
    graph_[vdesc].cat = category_t::ARRAY;
    // TODO: graph_[vdesc].type = ;
    create_scalar_at(vdesc);
    break;
  }
  case TGC_STRUCT: {
    graph_[vdesc].cat = category_t::STRUCT;
    // TODO: graph_[vdesc].type = ;
    int nchilds = cfg::get(config_, TG::NFIELDS);
    for (int i = 0; i < nchilds; ++i)
      create_scalar_at(vdesc);
    break;
  }
  default:
    throw(std::runtime_error("Unknown container"));
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