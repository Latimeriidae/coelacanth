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

// option as probability function
struct probf {
  probf_t probs;
};

// option that equals to 0 or 1 depending of probability (m out of n chances)
struct pflag {
  int prob;
  int total;
};

// option record is variant out of all option types
using optrecord = std::variant<single, diap, probf, pflag>;

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
  config(int seed, bool quiet, ormap_cit start, ormap_cit fin)
      : cfg_(start, fin), quiet_(quiet), mt_source(seed) {}
  int get(int id) const;
  size_t prob_size(int id) const;
  bool quiet() const { return quiet_; }
  ormap_cit cbegin() const { return cfg_.cbegin(); }
  ormap_cit cend() const { return cfg_.cend(); }
  void dump(std::ostream &os) const;
  int rand_positive() const {
    return rand_from(0, std::numeric_limits<int>::max());
  }
};

template <typename T> int get(const config &cfg, T id) {
  return cfg.get(static_cast<int>(id));
}

template <typename T> size_t prob_size(const config &cfg, T id) {
  return cfg.prob_size(static_cast<int>(id));
}

config read_global_config(int argc, char **argv);

//------------------------------------------------------------------------------
//
// Config adapter to make rng out of rng, hidden in cfg
//
//------------------------------------------------------------------------------

struct config_rng {
  using result_type = int;
  cfg::config &cf_;
  config_rng(const cfg::config &cf) : cf_(const_cast<cfg::config &>(cf)) {}
  result_type operator()() { return cf_.rand_positive(); }
  int min() { return 0; }
  int max() { return std::numeric_limits<int>::max(); }
};

} // namespace cfg
