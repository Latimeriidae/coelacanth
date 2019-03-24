//------------------------------------------------------------------------------
//
// High-level abstract interface for configuration
//
// Every packaged task have its own config
//
// config includes:
// (1) random number generator (seeded by producer thread)
// (2) mapping from option ID to variant class
//
// For convenience method get() returns (possibly random) value of option
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

// option enum for get
#define OPTENUM
#include "options.h"
#undef OPTENUM

// different option types
namespace cfg {

// option with single value
struct single {
  int val;
};

// option with value from diap
struct diap {
  int from;
  int to;
};

using probf_t = std::vector<int>;
using probf_it = std::vector<int>::iterator;
using probf_cit = std::vector<int>::const_iterator;

struct probf {
  probf_t probs;
};

// option record is variant out of all option types
using optrecord = std::variant<single, diap, probf>;

} // namespace cfg

namespace cfg {

using ormap_t = std::map<int, optrecord>;
using ormap_it = std::map<int, optrecord>::iterator;
using ormap_cit = std::map<int, optrecord>::const_iterator;

// main config class
class config {
  ormap_t cfg_;
  bool quiet_;
  mutable std::mt19937_64 mt_source;

  // get helpers
private:
  int from_probf(probf_cit start, probf_cit fin) const;
  int rand_from(int min, int max) const;

  // public interface
public:
  config(int seed, bool quiet, ormap_it start, ormap_it fin)
      : cfg_(start, fin), quiet_(quiet), mt_source(seed) {}
  int get(int id) const;
  bool quiet() const { return quiet_; }
  ormap_cit cbegin() const { return cfg_.cbegin(); }
  ormap_cit cend() const { return cfg_.cend(); }
  void dump(std::ostream &os) const;
};

config read_global_config(int argc, char **argv);

} // namespace cfg
