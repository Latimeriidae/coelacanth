//------------------------------------------------------------------------------
//
// tasksystem.h -- task system for coelacanth test generator
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

#include "configs.h"

//------------------------------------------------------------------------------
//
// forward declarations
//
//------------------------------------------------------------------------------

// callgraph
namespace tg {
class typegraph_t;
}

using tg_task_type = std::shared_ptr<tg::typegraph_t>(int, const cfg::config &);

std::shared_ptr<tg::typegraph_t> typegraph_create(int, const cfg::config &);
void typegraph_dump(std::shared_ptr<tg::typegraph_t>, std::ostream &);

// callgraph
namespace cg {
class callgraph_t;
}

using cg_task_type = std::shared_ptr<cg::callgraph_t>(
    int, const cfg::config &, std::shared_ptr<tg::typegraph_t>);

std::shared_ptr<cg::callgraph_t>
callgraph_create(int, const cfg::config &, std::shared_ptr<tg::typegraph_t>);

// varassign
namespace va {
class varassign;
}

using va_task_type =
    std::shared_ptr<va::varassign>(int, std::shared_ptr<tg::typegraph_t>);

std::shared_ptr<va::varassign> vassign_create(int,
                                              std::shared_ptr<tg::typegraph_t>);

// controlgraph

// locIR

// exprIR

// langprinter

//------------------------------------------------------------------------------
//
// technical support
//
//------------------------------------------------------------------------------

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
  R2 operator()(Args &&... args) && {
    R2 ret = invoke(ptr.get(), std::forward<Args>(args)...);
    clear();
    return std::move(ret);
  }

  // invoke if R is void
  template <typename R2 = R,
            std::enable_if_t<std::is_same<R2, void>{}, int> = 0>
  R2 operator()(Args &&... args) && {
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

// generic creation of task and its future
// for example:
// create_task(callgraph_create, default_config);
// will return pair of task and future for std::shared_ptr<cg::callgraph_t>
template <typename TT, typename F, typename... Args>
auto create_task(F f, Args &&... args) {
  std::packaged_task<TT> tsk{f};
  auto fut = tsk.get_future();
  task_t t{[ct = std::move(tsk),
            args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
    std::apply([ct = std::move(ct)](auto &&... args) mutable { ct(args...); },
               std::move(args));
    return 0;
  }};

  return std::make_pair(std::move(t), std::move(fut));
}
