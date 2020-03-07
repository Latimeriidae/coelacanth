//------------------------------------------------------------------------------
//
// multi-thread debug output
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <mutex>
#include <thread>

struct dbgs {
  dbgs() = default;
  static std::mutex mut_dbgs;

  template <typename T> dbgs &&outr(T t) && {
    std::lock_guard<std::mutex> outlk{mut_dbgs};
    std::cout << t;
    return std::move(*this);
  }

  template <typename T> dbgs &out(T t) {
    std::lock_guard<std::mutex> outlk{mut_dbgs};
    std::cout << t;
    return *this;
  }
};

template <typename T> dbgs &&operator<<(dbgs &&d, T t) {
  return std::move(d).outr(t);
}

template <typename T> dbgs &operator<<(dbgs &d, T t) { return d.out(t); }
