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

#include <fstream>
#include <future>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <utility>

#include "config/configs.h"
#include "fireonce.h"

//------------------------------------------------------------------------------
//
// forward declarations
//
//------------------------------------------------------------------------------

// consumer thread function
void consumer_thread_func();

// push sentinel task to global queue
void push_sentinel_task();

// push task to global queue
void push_task(task_t);

// callgraph
namespace tg {
class typegraph_t;
}

using tg_task_type = std::shared_ptr<tg::typegraph_t>(int, const cfg::config &);
using tg_read_task_type = std::shared_ptr<tg::typegraph_t>(std::string,
                                                           const cfg::config &);
using typegraph_future_t =
    decltype(std::packaged_task<tg_task_type>{}.get_future());
using typegraph_sp_t = decltype(typegraph_future_t{}.get());

std::shared_ptr<tg::typegraph_t> typegraph_create(int, const cfg::config &);
std::shared_ptr<tg::typegraph_t> typegraph_read(std::string,
                                                const cfg::config &);
void typegraph_dump(std::shared_ptr<tg::typegraph_t>, std::ostream &);

// callgraph
namespace cg {
class callgraph_t;
}

using cg_task_type = std::shared_ptr<cg::callgraph_t>(
    int, const cfg::config &, std::shared_ptr<tg::typegraph_t>);
using callgraph_future_t =
    decltype(std::packaged_task<cg_task_type>{}.get_future());
using callgraph_sp_t = decltype(callgraph_future_t{}.get());

std::shared_ptr<cg::callgraph_t>
callgraph_create(int, const cfg::config &, std::shared_ptr<tg::typegraph_t>);
void callgraph_dump(std::shared_ptr<cg::callgraph_t> cg, std::ostream &os);

// varassign
namespace va {
class varassign_t;
}

using va_task_type = std::shared_ptr<va::varassign_t>(
    int, const cfg::config &, std::shared_ptr<tg::typegraph_t>,
    std::shared_ptr<cg::callgraph_t>);
using varassign_future_t =
    decltype(std::packaged_task<va_task_type>{}.get_future());
using varassign_sp_t = decltype(varassign_future_t{}.get());

std::shared_ptr<va::varassign_t>
varassign_create(int, const cfg::config &, std::shared_ptr<tg::typegraph_t>,
                 std::shared_ptr<cg::callgraph_t>);
void varassign_dump(std::shared_ptr<va::varassign_t>, std::ostream &);

// controlgraph
namespace cn {
class controlgraph_t;
}

using cn_task_type = std::shared_ptr<cn::controlgraph_t>(
    int, const cfg::config &, std::shared_ptr<tg::typegraph_t>,
    std::shared_ptr<cg::callgraph_t>, std::shared_ptr<va::varassign_t>);
using contgraph_future_t =
    decltype(std::packaged_task<cn_task_type>{}.get_future());
using contgraph_sp_t = decltype(contgraph_future_t{}.get());

std::shared_ptr<cn::controlgraph_t>
controlgraph_create(int, const cfg::config &, std::shared_ptr<tg::typegraph_t>,
                    std::shared_ptr<cg::callgraph_t>,
                    std::shared_ptr<va::varassign_t>);

void controlgraph_dump(std::shared_ptr<cn::controlgraph_t>, std::ostream &os);

// locIR

// exprIR

// langprinter

//------------------------------------------------------------------------------
//
// technical support
//
//------------------------------------------------------------------------------

// generic creation of task and its future
// for example:
// create_task(callgraph_create, default_config);
// will return pair of task and future for std::shared_ptr<cg::callgraph_t>
template <typename F, typename... Args> auto create_task(F f, Args &&...args) {
  std::packaged_task<std::remove_pointer_t<F>> tsk{f};
  auto fut = tsk.get_future();
  task_t t{[ct = std::move(tsk),
            args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
    std::apply([ct = std::move(ct)](auto &&...args) mutable { ct(args...); },
               std::move(args));
    return 0;
  }};

  return std::make_pair(std::move(t), std::move(fut));
}
