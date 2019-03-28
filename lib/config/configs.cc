//------------------------------------------------------------------------------
//
// Configuration impl
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <algorithm>
#include <boost\program_options.hpp>
#include <ctime>
#include <map>
#include <random>
#include <string>
#include <utility>

#include "configs.h"

using std::map;
using std::string;
using std::uniform_int_distribution;

namespace po = boost::program_options;

// helper for variant visitor
template <class T> struct always_false : std::false_type {};

namespace cfg {

// boost options desc
static po::options_description desc("Allowed options");
static po::variables_map vm;

// global option storage
static map<int, optrecord> option_registry;
static map<string, int> options_byname;
static map<int, string> options_names;
static map<int, string> option_descriptions;

void register_option(int global_id, string global_name, optrecord optrec,
                     string description) {
  std::transform(global_name.begin(), global_name.end(), global_name.begin(),
                 ::tolower);
  option_registry[global_id] = optrec;
  options_byname[global_name] = global_id;
  options_names[global_id] = global_name;
  option_descriptions[global_id] = description;

  // adding command-line option
  std::visit(
      [global_name, description](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, cfg::single>)
          desc.add_options()(global_name.c_str(), po::value<int>(),
                             description.c_str());
        else if constexpr (std::is_same_v<T, cfg::diap>) {
          string gnmax = global_name + "_max";
          string gnmin = global_name + "_min";
          desc.add_options()(gnmax.c_str(), po::value<int>(),
                             description.c_str());
          desc.add_options()(gnmin.c_str(), po::value<int>(),
                             description.c_str());
        } else if constexpr (std::is_same_v<T, cfg::probf>) {
          desc.add_options()(global_name.c_str(),
                             po::value<std::vector<int>>()->multitoken(),
                             description.c_str());
        } else
          static_assert(always_false<T>::value, "non-exhaustive visitor!");
      },
      optrec);
}

void register_options() {
  desc.add_options()("help", "produce help message");
  desc.add_options()("seed", po::value<int>()->default_value(time(nullptr)),
                     "seed for RNG");
  desc.add_options()("quiet", po::bool_switch()->default_value(false),
                     "enable file overwrite");
#define OPREGISTRY
#include "options.h"
#undef OPREGISTRY
}

config read_global_config(int argc, char **argv) {
  // register default config
  register_options();

  // parse programm options, overwrite defaults
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  bool quiet = vm["quiet"].as<bool>();
  int seed = vm["seed"].as<int>();
  ;

  if (!quiet) {
    std::cout << "starting with seed = " << seed << std::endl;
  }

  for (auto &r : option_registry) {
    string name = options_names[r.first];
    string namemax = name + "_max";
    if ((0 == vm.count(name.c_str())) && (0 == vm.count(namemax.c_str())))
      continue;
    std::visit(
        [name](auto &arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, cfg::single>)
            arg.val = vm[name.c_str()].as<int>();
          else if constexpr (std::is_same_v<T, cfg::diap>) {
            string namemax = name + "_max";
            string namemin = name + "_min";
            arg.from = vm[namemin.c_str()].as<int>();
            arg.to = vm[namemax.c_str()].as<int>();
          } else if constexpr (std::is_same_v<T, cfg::probf>) {
            arg.probs = vm[name.c_str()].as<std::vector<int>>();
          } else
            static_assert(always_false<T>::value, "non-exhaustive visitor!");
        },
        r.second);
  }

  // default config ready
  config cfg(seed, quiet, option_registry.begin(), option_registry.end());

  return cfg;
}

// random value from probability function
// like [10, 50, 100]
// shall return 0 with prob = 10, 1 with prob = 40 and 2 with prob = 50
int config::from_probf(probf_cit start, probf_cit fin) const {
  if (start == fin)
    throw std::runtime_error("Probability function shall be non-empty");

  int sum = *(fin - 1);
  if (sum == 0)
    throw std::runtime_error("Probability function shall be normalizable");

  int val = rand_from(0, sum - 1);
  int curn = 0;
  while (start != fin) {
    if (*start > val)
      break;
    curn += 1;
    ++start;
  }

  return curn;
}

int config::rand_from(int min, int max) const {
  uniform_int_distribution<int> dist(min, max);
  return dist(mt_source);
}

int config::get(int id) const {
  auto fit = cfg_.find(id);
  if (fit == cfg_.end())
    throw std::runtime_error("Config have no such value");

  int retval = std::visit(
      [this](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, cfg::single>)
          return arg.val;
        else if constexpr (std::is_same_v<T, cfg::diap>)
          return rand_from(arg.from, arg.to);
        else if constexpr (std::is_same_v<T, cfg::probf>)
          return from_probf(arg.probs.begin(), arg.probs.end());
        else
          static_assert(always_false<T>::value, "non-exhaustive visitor!");
      },
      fit->second);

  return retval;
}

void config::dump(std::ostream &os) const { os << "Programm config:\n"; }

} // namespace cfg