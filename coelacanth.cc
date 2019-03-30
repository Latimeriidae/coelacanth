//------------------------------------------------------------------------------
//
// coelacanth.cc -- main driver for coelacanth test generator
//
// we have 4 randomization levels
// (1) varassign from typegraph                  (--nvar)
// (2) controlgraph from callgraph and varassign (--nsplits)
// (3) locIR from controlgraph                   (--nlocs)
// (4) exprIR from locIR                         (--narith)
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
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "configs.h"
#include "tasksystem.h"
#include "timestamp.h"

std::queue<task_t> task_queue;
std::mutex task_queue_mutex;

void consumer_thread_func();

int main(int argc, char **argv) {
  // default config
  cfg::config default_config = cfg::read_global_config(argc, argv);

  if (!default_config.quiet())
    std::cout << "Coelacanth built on " << TIMESTAMP << std::endl;

  {
    std::ofstream of("initial.cfg");
    default_config.dump(of);
  }

  // consumer threads
  std::vector<std::thread> consumers;
  auto nthreads = default_config.get(NCONSUMERS);

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
  auto tg = typegraph_fut.get();

  {
    std::ofstream of("initial.types");
    typegraph_dump(tg, of);
  }

  int cgseed = default_config.rand_positive();

  // put callgraph task
  auto &&[callgraph_task, callgraph_fut] =
      create_task<cg_task_type>(callgraph_create, cgseed, default_config, tg);

  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(callgraph_task));
  }

  int nvar = default_config.get(NVAR);
  int nsplits = default_config.get(NSPLITS);
  int nlocs = default_config.get(NLOCS);
  int narith = default_config.get(NARITH);

  // put varassign tasks
  for (int r_var = 0; r_var < nvar; ++r_var) {
    // TODO: put task to queue
  }

  // now we need callgraph to start creating controlgraphs
  auto cg = callgraph_fut.get();

  for (int r_var = 0; r_var < nvar; ++r_var) {
    // now we need #r_var's varassign to start creating controlgraphs
    // TODO: future.get

    // put controlgraph tasks
    for (int r_splits = 0; r_splits < nsplits; ++r_splits) {
      // TODO: put task to queue
    }
  }

  for (int r_var = 0; r_var < nvar; ++r_var)
    for (int r_splits = 0; r_splits < nsplits; ++r_splits)
      for (int r_locs = 0; r_locs < nlocs; ++r_locs) {
        // create locIR
        // TODO: put task to queue
      }

  for (int r_var = 0; r_var < nvar; ++r_var)
    for (int r_splits = 0; r_splits < nsplits; ++r_splits)
      for (int r_locs = 0; r_locs < nlocs; ++r_locs)
        for (int r_exprs = 0; r_exprs < narith; ++r_exprs) {
          // create exprIR
          // TODO: put task to queue
        }

  task_t sentinel{[] { return -1; }};
  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(sentinel));
  }

  for (int i = 0; i < nthreads; ++i)
    consumers[i].join();
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
