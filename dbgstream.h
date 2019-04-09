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

#include <mutex>
#include <thread>

extern std::mutex mut_dbgs;

struct dbgs {
  dbgs() = default;

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
