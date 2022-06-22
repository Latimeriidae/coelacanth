//------------------------------------------------------------------------------
//
// fireonce.h -- service class for one-off future
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>

template <typename T> class fire_once;

// critical part of task system: one-off function class to place packaged_task
template <typename R, typename... Args> class fire_once<R(Args...)> {
  std::unique_ptr<void, void (*)(void *)> ptr{nullptr, +[](void *) {}};
  R (*invoke)(void *, Args...) = nullptr;

public:
  fire_once() = default;
  fire_once(fire_once &&) = default;
  fire_once &operator=(fire_once &&) = default;

  // constructor from anything callable
  template <typename F> fire_once(F &&f) {
    auto pf = std::make_unique<F>(std::move(f));
    invoke = +[](void *pf, Args... args) -> R {
      F *f = reinterpret_cast<F *>(pf);
      return (*f)(std::forward<Args>(args)...);
    };
    ptr = {pf.release(), [](void *pf) {
             F *f = reinterpret_cast<F *>(pf);
             delete f;
           }};
  }

  // invoke if R not void
  template <typename R2 = R,
            std::enable_if_t<!std::is_same<R2, void>{}, int> = 0>
  R2 operator()(Args &&...args) && {
    R2 ret = invoke(ptr.get(), std::forward<Args>(args)...);
    clear();
    return std::move(ret);
  }

  // invoke if R is void
  template <typename R2 = R,
            std::enable_if_t<std::is_same<R2, void>{}, int> = 0>
  R2 operator()(Args &&...args) && {
    invoke(ptr.get(), std::forward<Args>(args)...);
    clear();
  }

  void clear() {
    invoke = nullptr;
    ptr.reset();
  }

  explicit operator bool() const { return static_cast<bool>(ptr); }
};

// generic (type-erased) task to put on queue
// returns -1 if it is special signalling task (end of work for consumers)
// otherwise do what it shall and return 0
using task_t = fire_once<int()>;
