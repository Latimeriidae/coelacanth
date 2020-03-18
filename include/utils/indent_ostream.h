//------------------------------------------------------------------------------
//
// Indentation output stream class definition.
//
// The indent_ostream class purpose is to provide simple interface
// for pretty printing program-like data. Mainly, it introduces
// new manipulators that can increase and descrease indentation level.
// This level then used to indent output each time newline character
// is printed. Thus, usage model is quite simple:
//
// indent_ostream(std::cout, 2 /* level spaces */) ios;
// ios << increase_indent << "Hello\nWorld\n" << decrease_indent << "!";
//
// This example will produce the following output:
// Hello
//   World
// !
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/operations.hpp>

#include <cassert>
#include <ostream>
#include <streambuf>

namespace utils {

class indent_filter_t : public boost::iostreams::output_filter {
  int level_spaces_;
  int cur_spaces_ = 0;
  int remaining_spaces_ = 0;

private:
  // If there was a request to change indentation level
  // then we should also change remaining spaces count
  // because if the request was just after a newline
  // we would emit wrong indentation.
  void correct_spaces() {
    if (remaining_spaces_ != 0)
      remaining_spaces_ = cur_spaces_;
  }

public:
  indent_filter_t(int level_spaces) : level_spaces_(level_spaces) {
    assert(level_spaces_ >= 0 && "Expected non-negative value");
  }

  template <typename Sink> bool put(Sink &sink, char c) {
    namespace io = boost::iostreams;

    if (c == '\n') {
      remaining_spaces_ = cur_spaces_;
      return io::put(sink, c);
    }

    while (remaining_spaces_ != 0) {
      // put operation can fail so we should decrement
      // spaces only if putting to sink was successful.
      if (!io::put(sink, ' '))
        return false;
      --remaining_spaces_;
    }
    return io::put(sink, c);
  }

  void increase_level(int num_levels = 1) {
    assert(num_levels > 0 && "Expected positive number");
    cur_spaces_ += level_spaces_ * num_levels;
    correct_spaces();
  }
  void decrease_level(int num_levels = 1) {
    assert(num_levels > 0 && "Expected positive number");
    cur_spaces_ -= level_spaces_ * num_levels;
    assert(cur_spaces_ >= 0 && "Indentation underflow");
    correct_spaces();
  }

  // Methods mostly for testing.
  void reset() { cur_spaces_ = 0; }
  int get_current_level() const {
    if (level_spaces_ == 0)
      return 0;
    return cur_spaces_ / level_spaces_;
  }
};

class indent_ostreambuf_t : public boost::iostreams::filtering_ostreambuf {
  using base = boost::iostreams::filtering_ostreambuf;

  static constexpr int filter_index_ = 0;

public:
  indent_ostreambuf_t(std::streambuf &buf, int level_spaces) : base{} {
    // Second argument controls buffer size.
    // We need no buffer for our streambuf.
    base::push(indent_filter_t{level_spaces}, 0);
    base::push(buf, 0);
  }

  void increase_level() const {
    component<indent_filter_t>(filter_index_)->increase_level();
  }
  void decrease_level() const {
    component<indent_filter_t>(filter_index_)->decrease_level();
  }
  int get_current_level() const {
    return component<indent_filter_t>(filter_index_)->get_current_level();
  }
};

class indent_ostream_t : public std::ostream {
  using base = std::ostream;

  // ios_base::xalloc index for this type.
  static const int x_stream_id_;

  indent_ostreambuf_t buf_;

public:
  indent_ostream_t(std::ostream &os, int level_spaces)
      : base{&buf_}, buf_{*os.rdbuf(), level_spaces} {
    pword(x_stream_id_) = static_cast<std::ostream *>(this);
  }

  static bool is_me(std::ostream &os) {
    return os.pword(x_stream_id_) == std::addressof(os);
  }

  void increase_level() const { buf_.increase_level(); }

  void decrease_level() const { buf_.decrease_level(); }

  int get_current_level() const { return buf_.get_current_level(); }
};

inline auto increase_indent = [](std::ostream &os) -> std::ostream & {
  if (!indent_ostream_t::is_me(os))
    return os;
  auto &ios = static_cast<indent_ostream_t &>(os);
  ios.increase_level();
  return ios;
};

inline auto decrease_indent = [](std::ostream &os) -> std::ostream & {
  if (!indent_ostream_t::is_me(os))
    return os;
  auto &ios = static_cast<indent_ostream_t &>(os);
  ios.decrease_level();
  return ios;
};

} // namespace utils
