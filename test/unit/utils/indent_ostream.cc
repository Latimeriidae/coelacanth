//------------------------------------------------------------------------------
//
// Basic tests for indentation output stream.
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "utils/indent_ostream.h"

#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <string>

auto make_null_stream() {
  return boost::iostreams::stream{boost::iostreams::null_sink{}};
}

BOOST_AUTO_TEST_SUITE(utils_tests)

BOOST_AUTO_TEST_SUITE(indent_ostream)

BOOST_AUTO_TEST_CASE(stream_identity) {
  auto null = make_null_stream();
  utils::indent_ostream_t ios{null, 2};
  BOOST_TEST(utils::indent_ostream_t::is_me(ios));
  BOOST_TEST(!utils::indent_ostream_t::is_me(null));
}

BOOST_AUTO_TEST_CASE(filter_manip) {
  utils::indent_filter_t ifilter{1};
  BOOST_TEST(ifilter.get_current_level() == 0);
  ifilter.increase_level();
  BOOST_TEST(ifilter.get_current_level() == 1);
  ifilter.increase_level();
  BOOST_TEST(ifilter.get_current_level() == 2);
  ifilter.decrease_level();
  BOOST_TEST(ifilter.get_current_level() == 1);
  ifilter.decrease_level();
  BOOST_TEST(ifilter.get_current_level() == 0);
  ifilter.increase_level(100);
  BOOST_TEST(ifilter.get_current_level() == 100);
  ifilter.reset();
  BOOST_TEST(ifilter.get_current_level() == 0);

  utils::indent_filter_t null_filter{0};
  BOOST_TEST(null_filter.get_current_level() == 0);
  null_filter.increase_level(100);
  BOOST_TEST(null_filter.get_current_level() == 0);
}

BOOST_AUTO_TEST_CASE(stream_manip) {
  auto null = make_null_stream();
  utils::indent_ostream_t ios{null, 2};
  BOOST_TEST(ios.get_current_level() == 0);
  ios << utils::increase_indent;
  BOOST_TEST(ios.get_current_level() == 1);
  ios << utils::increase_indent << utils::increase_indent;
  BOOST_TEST(ios.get_current_level() == 3);
  ios << utils::decrease_indent;
  BOOST_TEST(ios.get_current_level() == 2);
  ios << utils::decrease_indent << utils::decrease_indent;
  BOOST_TEST(ios.get_current_level() == 0);
}

BOOST_AUTO_TEST_CASE(null_filter) {
  namespace io = boost::iostreams;

  utils::indent_filter_t null_filter{0};
  std::string in{"Hello\nWorld\n!"};
  BOOST_TEST(io::test_output_filter(null_filter, in, in));
  null_filter.increase_level(1);
  BOOST_TEST(io::test_output_filter(null_filter, in, in));
}

BOOST_AUTO_TEST_CASE(filter) {
  namespace io = boost::iostreams;

  utils::indent_filter_t if1{1};
  std::string in{"Hello\nWorld\n!"};
  BOOST_TEST(io::test_output_filter(if1, in, in));

  if1.increase_level();
  std::string out1{"Hello\n World\n !"};
  BOOST_TEST(io::test_output_filter(if1, in, out1));

  if1.increase_level(4);
  std::string out2{"Hello\n     World\n     !"};
  BOOST_TEST(io::test_output_filter(if1, in, out2));

  if1.decrease_level();
  BOOST_TEST(io::test_output_filter(if1, out1, out2));

  utils::indent_filter_t if2{2};
  std::string in2{"Hello\n\nWorld\n\n\n!"};
  BOOST_TEST(io::test_output_filter(if2, in2, in2));

  if2.increase_level(2);
  std::string out21{"Hello\n\n    World\n\n\n    !"};
  BOOST_TEST(io::test_output_filter(if2, in2, out21));
}

BOOST_AUTO_TEST_CASE(stream) {
  using namespace utils;

  std::ostringstream oss;
  indent_ostream_t ios{oss, 2};

  ios << "Hello\nWorld" << std::endl << "!";
  BOOST_TEST(oss.str() == "Hello\nWorld\n!");

  oss = std::ostringstream{};
  ios << increase_indent << "Hello\nWorld\n" << decrease_indent << "!";
  BOOST_TEST(oss.str() == "Hello\n  World\n!");
}

BOOST_AUTO_TEST_CASE(ignored_manip) {
  using namespace utils;

  std::ostringstream oss;
  oss << increase_indent << "Hello\nWorld\n" << decrease_indent << "!";
  BOOST_TEST(oss.str() == "Hello\nWorld\n!");
}

BOOST_AUTO_TEST_CASE(double_filter) {
  using namespace utils;

  std::ostringstream oss;
  indent_ostream_t ios1{oss, 2};
  indent_ostream_t ios2{ios1, 1};

  ios2 << "Hello\nWorld\n!";
  BOOST_TEST(oss.str() == "Hello\nWorld\n!");

  ios1.increase_level();
  oss = std::ostringstream{};
  ios2 << "Hello\nWorld\n!";
  BOOST_TEST(oss.str() == "Hello\n  World\n  !");

  oss = std::ostringstream{};
  ios2 << increase_indent << "Hello\nWorld\n" << decrease_indent << "!";
  BOOST_TEST(oss.str() == "Hello\n   World\n  !");

  oss = std::ostringstream{};
  ios2 << increase_indent << "Hello\nWorld\n"
       << decrease_indent << (ios1.decrease_level(), "!");
  BOOST_TEST(oss.str() == "Hello\n   World\n!");
}

BOOST_AUTO_TEST_SUITE_END() // indent_ostream

BOOST_AUTO_TEST_SUITE_END() // utils_tests
