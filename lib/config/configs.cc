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
  boost::replace_all(global_name, "_", "-");
  option_registry[global_id] = optrec;
  options_byname[global_name] = global_id;
  options_names[global_id] = global_name;
  option_descriptions[global_id] = description;

  // adding command-line option
  std::visit(
      [global_name, description, borderval](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, cfg::single>) {
          desc.add_options()(global_name.c_str(),
                             po::value<int>()->default_value(arg.val),
                             description.c_str());
        } else if constexpr (std::is_same_v<T, cfg::single_bool>) {
          assert(arg.val == false);
          string no_name = string("no-") + global_name;
          string no_desc = description + " (switch off)";
          desc.add_options()(global_name.c_str(),
                             po::bool_switch()->default_value(arg.val),
                             description.c_str());
          desc.add_options()(no_name.c_str(),
                             po::bool_switch()->default_value(!arg.val),
                             no_desc.c_str());
        } else if constexpr (std::is_same_v<T, cfg::single_string>) {
          desc.add_options()(global_name.c_str(),
                             po::value<string>()->default_value(arg.val),
                             description.c_str());
        } else if constexpr (std::is_same_v<T, cfg::diap>) {
          string gnmax = global_name + "-max";
          string gnmin = global_name + "-min";
          string descmax = description + " (max value)";
          string descmin = description + " (min value)";
          desc.add_options()(gnmax.c_str(),
                             po::value<int>()->default_value(arg.to),
                             descmax.c_str());
          desc.add_options()(gnmin.c_str(),
                             po::value<int>()->default_value(arg.from),
                             descmin.c_str());
        } else if constexpr (std::is_same_v<T, cfg::probf>) {
          std::ostringstream descprobf;
          descprobf << description << ". Defaults to:";
          for (auto p : arg.probs)
            descprobf << " " << p;
          desc.add_options()(global_name.c_str(),
                             po::value<std::vector<int>>()->multitoken(),
                             descprobf.str().c_str());
          probf_borders[global_name] = borderval;
        } else if constexpr (std::is_same_v<T, cfg::pflag>) {
          std::ostringstream descstr;
          descstr << description << ". Total is: " << arg.total;
          desc.add_options()(global_name.c_str(),
                             po::value<int>()->default_value(arg.prob),
                             descstr.str().c_str());
          probf_borders[global_name] = borderval;
        } else {
          static_assert(always_false<T>::value, "non-exhaustive visitor!");
        }
      },
      optrec);
} // namespace cfg

void register_options() {
  desc.add_options()("help", "Produce help message");
  desc.add_options()("seed", po::value<int>()->default_value(time(nullptr)),
                     "Seed for RNG");
  desc.add_options()("quiet", po::bool_switch()->default_value(false),
                     "Suppress almost all messages");
  desc.add_options()("dumps", po::bool_switch()->default_value(false),
                     "Make coelacanth emit verbose dumps from all passes");
  desc.add_options()("showval", po::value<std::string>()->default_value("none"),
                     "Show value of given option (mostly debugging purposes)");
#define OPREGISTRY
#include "options.h"
#undef OPREGISTRY
}

// Additional command line parser which interprets '-no-something' as a
// option "something" with the value "false"
// TODO: now it is switched off, shall be enabled later
std::pair<std::string, std::string> at_option_parser(const std::string &s) {
  abort();
  if (s.substr(0, 3) == std::string("-no"))
    return std::make_pair(s.substr(3), std::string("false"));
  return {};
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
        (0 == vm.count(namemin.c_str()))) {
      continue;
    }
    if (vm.count(namemax.c_str()) != vm.count(namemin.c_str())) {
      std::ostringstream s;
      s << "Problems with " << name << ". You shall specify both options "
        << namemin << " and " << namemax << " or none of them" << std::endl;
      throw std::runtime_error(s.str());
    }
    std::visit(
        [name, namemax, namemin](auto &arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, cfg::single>) {
            arg.val = vm[name.c_str()].as<int>();
          } else if constexpr (std::is_same_v<T, cfg::single_bool>) {
            arg.val = vm[name.c_str()].as<bool>();
          } else if constexpr (std::is_same_v<T, cfg::single_string>) {
            arg.val = vm[name.c_str()].as<std::string>();
          } else if constexpr (std::is_same_v<T, cfg::diap>) {
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
          } else {
            static_assert(always_false<T>::value, "non-exhaustive visitor!");
          }
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
  std::lock_guard lk{mt_mutex};
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
        else if constexpr (std::is_same_v<T, cfg::single_bool>)
          return arg.val ? 1 : 0;
        else if constexpr (std::is_same_v<T, cfg::single_string>)
          return std::stoi(arg.val);
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

std::string config::gets(int id) const {
  auto fit = cfg_.find(id);
  if (fit == cfg_.end())
    throw std::runtime_error("Config have no such value");

  std::string retval = std::visit(
      [this](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, cfg::single>)
          return std::to_string(arg.val);
        else if constexpr (std::is_same_v<T, cfg::single_bool>)
          return std::to_string(arg.val);
        else if constexpr (std::is_same_v<T, cfg::single_string>)
          return arg.val;
        else if constexpr (std::is_same_v<T, cfg::diap>)
          return std::to_string(rand_from(arg.from, arg.to));
        else if constexpr (std::is_same_v<T, cfg::pflag>)
          return (rand_from(0, arg.total) < arg.prob) ? std::string("1")
                                                      : std::string("0");
        else if constexpr (std::is_same_v<T, cfg::probf>)
          return std::to_string(from_probf(arg.probs.begin(), arg.probs.end()));
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
