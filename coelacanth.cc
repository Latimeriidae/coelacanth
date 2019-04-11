//------------------------------------------------------------------------------
//
// coelacanth.cc -- main driver for coelacanth test generator
//
// we have 4 randomization levels
// (1) varassign from typegraph                  (--pg-var)
// (2) controlgraph from callgraph and varassign (--pg-splits)
// (3) locIR from controlgraph                   (--pg-locs)
// (4) exprIR from locIR                         (--pg-arith)
//
// Main sequence is putting tasks on queue and getting required futures
//
// High level order is:
// 1. Read options and create global config object
// 2. Create consumer threads and start
// 3. Put all tasks for
//

//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "configs.h"
#include "tasksystem.h"
#include "timestamp.h"
#include "version.h"

std::queue<task_t> task_queue;
std::mutex task_queue_mutex;
std::mutex mut_dbgs;

void consumer_thread_func();

int main(int argc, char **argv) {
  // default config
  cfg::config default_config = cfg::read_global_config(argc, argv);

  if (!default_config.quiet())
    std::cout << "Coelacanth info: git hash = " << GIT_COMMIT_HASH
              << ", built on " << TIMESTAMP << std::endl;

  {
    std::ofstream of("initial.cfg");
    default_config.dump(of);
  }

  // consumer threads
  std::vector<std::thread> consumers;
  auto nthreads = cfg::get(default_config, PG::CONSUMERS);

  if (!default_config.quiet())
    std::cout << "Starting " << nthreads << " consumer threads" << std::endl;

  for (int i = 0; i < nthreads; ++i)
    consumers.emplace_back(consumer_thread_func);

  // put typegraph task
  int tgseed = default_config.rand_positive();

  auto &&[typegraph_task, typegraph_fut] =
      create_task<tg_task_type>(typegraph_create, tgseed, default_config);

  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(typegraph_task));
  }

  // now we need typegraph to create callgraph
  auto tgraph = typegraph_fut.get();

  {
    std::ofstream of("initial.types");
    typegraph_dump(tgraph, of);
  }

  int cgseed = default_config.rand_positive();

  // put callgraph task
  auto &&[callgraph_task, callgraph_fut] = create_task<cg_task_type>(
      callgraph_create, cgseed, default_config, tgraph);

  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(callgraph_task));
  }

  int nvar = cfg::get(default_config, PG::VAR);
  int nsplits = cfg::get(default_config, PG::SPLITS);
  int nlocs = cfg::get(default_config, PG::LOCS);
  int narith = cfg::get(default_config, PG::ARITH);

  // now we need callgraph to start creating varassigns
  auto cgraph = callgraph_fut.get();

  {
    std::ofstream of("initial.calls");
    callgraph_dump(cgraph, of);
  }

  using va_fut_t =
      decltype(create_task<va_task_type>(varassign_create, 0, default_config,
                                         tgraph, cgraph)
                   .second);
  using va_sp_t = decltype(va_fut_t{}.get());

  std::vector<va_fut_t> vafuts;
  std::vector<va_sp_t> vassigns;

  // put varassign tasks
  for (int r_var = 0; r_var < nvar; ++r_var) {
    int vaseed = default_config.rand_positive();
    auto &&[vassign_task, vassign_fut] = create_task<va_task_type>(
        varassign_create, vaseed, default_config, tgraph, cgraph);
    {
      std::lock_guard<std::mutex> lk{task_queue_mutex};
      task_queue.push(std::move(vassign_task));
    }
    vafuts.emplace_back(std::move(vassign_fut));
  }

  using cn_fut_t =
      decltype(create_task<cn_task_type>(controlgraph_create, 0, default_config,
                                         tgraph, cgraph, va_sp_t{})
                   .second);
  using cn_sp_t = decltype(cn_fut_t{}.get());

  std::vector<cn_fut_t> cnfuts;
  std::vector<cn_sp_t> cfgs;

  // put controlgraph tasks
  for (int r_var = 0; r_var < nvar; ++r_var) {
    auto vassign = vafuts[r_var].get();

    std::ostringstream os;
    os << "varassign." << r_var;
    std::ofstream of(os.str());
    varassign_dump(vassign, of);

    for (int r_splits = 0; r_splits < nsplits; ++r_splits) {
      int cnseed = default_config.rand_positive();
      auto &&[cn_task, cn_fut] = create_task<cn_task_type>(
          controlgraph_create, cnseed, default_config, tgraph, cgraph, vassign);
      {
        std::lock_guard<std::mutex> lk{task_queue_mutex};
        task_queue.push(std::move(cn_task));
      }
      cnfuts.emplace_back(std::move(cn_fut));
    }

    vassigns.emplace_back(std::move(vassign));
  }

  // put locIR tasks
  for (int r_var = 0; r_var < nvar; ++r_var)
    for (int r_splits = 0; r_splits < nsplits; ++r_splits) {
      auto vassign = vassigns[r_var];
      cn_sp_t cfgraph;
      if (r_var == 0)
        cfgraph = cnfuts[r_splits].get();
      else
        cfgraph = cfgs[r_splits];

      for (int r_locs = 0; r_locs < nlocs; ++r_locs) {
        // create locIR
        // TODO: put task to queue
      }

      if (r_var == 0)
        cfgs.emplace_back(std::move(cfgraph));
    }

  // put exprIR tasks
  for (int r_var = 0; r_var < nvar; ++r_var)
    for (int r_splits = 0; r_splits < nsplits; ++r_splits)
      for (int r_locs = 0; r_locs < nlocs; ++r_locs)
        for (int r_exprs = 0; r_exprs < narith; ++r_exprs) {
          // create exprIR
          // TODO: put task to queue
        }

  // put final task
  task_t sentinel{[] { return -1; }};
  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(sentinel));
  }

  for (int i = 0; i < nthreads; ++i)
    consumers[i].join();

  if (!default_config.quiet())
    std::cout << "Done" << std::endl;
}

void consumer_thread_func() {
  for (;;) {
    task_t cur;
    {
      std::lock_guard<std::mutex> lk{task_queue_mutex};
      if (task_queue.empty())
        continue;
      cur = std::move(task_queue.front());
      task_queue.pop();
    }
    int res = std::move(cur)();
    if (res == -1) {
      task_t sentinel{[] { return -1; }};
      std::lock_guard<std::mutex> lk{task_queue_mutex};
      task_queue.push(std::move(sentinel));
      return;
    }
  }
}
