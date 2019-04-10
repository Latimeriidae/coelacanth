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
// Iterators
// no iterator can be used to iterate backwards
// all iterators have same value type: pair of (vertex descriptor, common type)
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/random.hpp>

#include "config/configs.h"
#include "typecats.h"
#include "typeiters.h"

namespace tg {

class typegraph_t final {
  cfg::config config_;
  tgraph_t graph_;

  // "Big" scalar types like int or long long. Not to query scalar nodes
  std::vector<scalar_desc_t> scalars_;

  // support sets (for easy access)
private:
  std::set<vertex_t> struct_vs_;
  std::set<vertex_t> array_vs_;
  std::set<vertex_t> pointer_vs_;

  // array to use in splits, consists of scalars only
  std::set<vertex_t> leaf_vs_;

  // subset of array_vs_: arrays with integral part
  // indexed by number of elements
  std::vector<std::vector<vertex_t>> perm_vs_;

  // subset of leaf_vs_: integral scalar leafs
  std::set<vertex_t> idx_vs_;

  // typegraph public interface
public:
  explicit typegraph_t(cfg::config &&);

  // vertex iterator (by descriptions)
  vertex_iter_t begin() const;
  vertex_iter_t end() const;

  // vertex properties from vertex descriptor
  vertexprop_t vertex_from(vertex_t v) const { return graph_[v]; }

  // property iterator
  ct_iterator_t begin_types() const;
  ct_iterator_t end_types() const;

  // vertex childs iterator
  child_iterator_t begin_childs(vertex_t v) const;
  child_iterator_t end_childs(vertex_t v) const;

  // debug dump in given stream
  void dump(std::ostream &) const;

  // type analysis helper
  int ntypes() const { return boost::num_vertices(graph_); }

  // random getters public interface
public:
  vertexprop_t get_random_type() const;

  // random type, that can be used as index (like int)
  vertexprop_t get_random_index_type() const;

  // random type, that can be used as permutation (like array of int)
  vertexprop_t get_random_perm_type(int nelems) const;

  // convenience getters
public:
  vertexprop_t get_pointee(vertex_t v) const;

  // public but not recommended for unenlightened use
public:
  vertex_t dest_from(edge_t e) const { return boost::target(e, graph_); }

  // typegraph construction helpers
private:
  void init_scalars();
  vertex_t create_scalar();
  void create_scalar_at(vertex_t parent);
  std::optional<vertex_t> get_pred(vertex_t v);
  void perform_splits();
  int do_split();
  int split_at(vertex_t vdesc);
  void unify_subscalars(std::set<vertex_t> &vsset);
  void process_pointer(vertex_t v);
  void create_bitfields();
  void choose_perms_idxs();
};

} // namespace tg
