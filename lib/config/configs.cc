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
#include <boost/algorithm/string/replace.hpp>
#include <boost/program_options.hpp>
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
static map<string, int> probf_borders;

void register_option(int global_id, string global_name, optrecord optrec,
                     string description, int borderval = 0) {
  std::transform(global_name.begin(), global_name.end(), global_name.begin(),
                 ::tolower);
  boost::replace_all(global_name, "::", "-");
  option_registry[global_id] = optrec;
  options_byname[global_name] = global_id;
  options_names[global_id] = global_name;
  option_descriptions[global_id] = description;

  // adding command-line option
  std::visit(
      [global_name, description, borderval](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, cfg::single>)
          desc.add_options()(global_name.c_str(), po::value<int>(),
                             description.c_str());
        else if constexpr (std::is_same_v<T, cfg::diap>) {
          string gnmax = global_name + "-max";
          string gnmin = global_name + "-min";
          string descmax = description + " (max value)";
          string descmin = description + " (min value)";
          desc.add_options()(gnmax.c_str(), po::value<int>(), descmax.c_str());
          desc.add_options()(gnmin.c_str(), po::value<int>(), descmin.c_str());
        } else if constexpr (std::is_same_v<T, cfg::probf>) {
          string descprobf = description + " (array)";
          desc.add_options()(global_name.c_str(),
                             po::value<std::vector<int>>()->multitoken(),
                             descprobf.c_str());
          probf_borders[global_name] = borderval;
        } else if constexpr (std::is_same_v<T, cfg::pflag>) {
          desc.add_options()(global_name.c_str(), po::value<int>(),
                             description.c_str());
          probf_borders[global_name] = borderval;
        } else
          static_assert(always_false<T>::value, "non-exhaustive visitor!");
      },
      optrec);
}

void register_options() {
  desc.add_options()("help", "Produce help message");
  desc.add_options()("seed", po::value<int>()->default_value(time(nullptr)),
                     "Seed for RNG");
  desc.add_options()("quiet", po::bool_switch()->default_value(false),
                     "Suppress almost all messages");
  desc.add_options()("dumps", po::bool_switch()->default_value(false),
                     "Make coelacanth emit verbose dumps from all passes");
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
  bool dumps = vm["dumps"].as<bool>();
  int seed = vm["seed"].as<int>();

  if (!quiet) {
    std::cout << "Coelacanth info: run with --help for option list"
              << std::endl;
    std::cout << "Coelacanth info: starting with seed = " << seed << std::endl;
  }

  for (auto &r : option_registry) {
    string name = options_names[r.first];
    string namemax = name + "-max";
    string namemin = name + "-min";
    if ((0 == vm.count(name.c_str())) && (0 == vm.count(namemax.c_str())) &&
        (0 == vm.count(namemin.c_str())))
      continue;
    if (vm.count(namemax.c_str()) != vm.count(namemin.c_str())) {
      std::ostringstream s;
      s << "Problems with " << name << ". You shall specify both options "
        << namemin << " and " << namemax << " or none of them" << std::endl;
      throw std::runtime_error(s.str());
    }
    std::visit(
        [name, namemax, namemin](auto &arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, cfg::single>)
            arg.val = vm[name.c_str()].as<int>();
          else if constexpr (std::is_same_v<T, cfg::diap>) {
            arg.from = vm[namemin.c_str()].as<int>();
            arg.to = vm[namemax.c_str()].as<int>();
          } else if constexpr (std::is_same_v<T, cfg::pflag>) {
            arg.prob = vm[name.c_str()].as<int>();
            arg.total = probf_borders[name];
          } else if constexpr (std::is_same_v<T, cfg::probf>) {
            arg.probs = vm[name.c_str()].as<std::vector<int>>();
            int nbords = probf_borders[name];
            if (arg.probs.size() != size_t(nbords)) {
              std::ostringstream s;
              s << "Problems with " << name << ". There are "
                << arg.probs.size() << " arguments but " << nbords
                << " entries in discrete probability function";
              throw std::runtime_error(s.str());
            }
          } else
            static_assert(always_false<T>::value, "non-exhaustive visitor!");
        },
        r.second);
  }

  // default config ready
  config cfg(seed, quiet, dumps, option_registry.begin(),
             option_registry.end());

  postverify(cfg);

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
        else if constexpr (std::is_same_v<T, cfg::pflag>)
          return (rand_from(0, arg.total) < arg.prob) ? 1 : 0;
        else if constexpr (std::is_same_v<T, cfg::probf>)
          return from_probf(arg.probs.begin(), arg.probs.end());
        else
          static_assert(always_false<T>::value, "non-exhaustive visitor!");
      },
      fit->second);

  return retval;
}

std::pair<int, int> config::minmax(int id) const {
  auto fit = cfg_.find(id);
  if (fit == cfg_.end())
    throw std::runtime_error("Config have no such value");
  const cfg::diap &d = std::get<cfg::diap>(fit->second);
  return std::make_pair(d.from, d.to);
}

size_t config::prob_size(int id) const {
  auto fit = cfg_.find(id);
  if (fit == cfg_.end())
    throw std::runtime_error("Config have no such value");
  const cfg::probf &prob = std::get<cfg::probf>(fit->second);
  return prob.probs.size();
}

void config::dump(std::ostream &os) const { os << "Programm config:\n"; }

// post-verification of config
void postverify(const config &) {
  // TODO: here we shall ensure dependencies between options
}

} // namespace cfg