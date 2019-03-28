//------------------------------------------------------------------------------
//
// High-level type-graph value-type
//
// type-graph consists of types:
// scalar types (integer, fp, etc)
// array types
// structure types
// pointers
//
// from user perspective most often query to typegraph is query of random type
// with certain limitations
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "config/configs.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

namespace tg {

//------------------------------------------------------------------------------
//
// Type categories support
//
//------------------------------------------------------------------------------

enum class category_t : unsigned char { SCALAR, STRUCT, ARRAY, POINTER };

struct scalar_desc_t {
  std::string name;
  int size;
  bool is_float;
  bool is_signed;

  scalar_desc_t(const char *nm, int sz, bool isf, bool iss)
      : name(nm), size(sz), is_float(isf), is_signed(iss) {}
};

struct scalar_t {
  int scalar_no;
};

struct struct_t {
  std::vector<bool> bitfields_;
};

struct array_t {
  int nitems;
};

struct pointer_t {};

struct vertexprop_t {
  int id;
  category_t cat;
  std::variant<scalar_t, struct_t, array_t, pointer_t> type;
  std::string get_name() const;
};

std::ostream &operator<<(std::ostream &os, vertexprop_t v);

//------------------------------------------------------------------------------
//
// Typegraph
//
//------------------------------------------------------------------------------

using tgraph_t = boost::adjacency_list<boost::vecS, boost::vecS,
                                       boost::directedS, vertexprop_t>;
using vertex_iter_t = boost::graph_traits<tgraph_t>::vertex_iterator;
using edge_iter_t = boost::graph_traits<tgraph_t>::edge_iterator;
using vertex_t = boost::graph_traits<tgraph_t>::vertex_descriptor;

class typegraph_t {
  cfg::config config_;
  tgraph_t graph_;
  std::set<vertex_t> leafs_;
  std::vector<scalar_desc_t> scalars_;

  // typegraph public interface
public:
  typegraph_t(cfg::config &&);
  void dump(std::ostream &os);

  // typegraph construction helpers
private:
  void init_scalars();
  vertex_t create_scalar();
  void create_scalar_at(vertex_t parent);
  void do_split();
};

} // namespace tg
