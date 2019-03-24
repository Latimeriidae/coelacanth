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

std::queue<task_t> task_queue;
std::mutex task_queue_mutex;

void consumer_thread_func();

int main(int argc, char **argv) {
  // 1. Read options and create global config object
  cfg::config default_config = cfg::read_global_config(argc, argv);

  {
    std::ofstream of("initial.cfg");
    default_config.dump(of);
  }

  // 3. Create consumer threads and start
  std::vector<std::thread> consumers;
  auto nthreads = default_config.get(NCONSUMERS);
  
  if (!default_config.quiet())
    std::cout << "Starting " << nthreads << " consumer threads" << std::endl;
  
  for (int i = 0; i < nthreads; ++i)
    consumers.emplace_back(consumer_thread_func);

  // 4. Put tasks

  auto &&[callgraph_task, callgraph_fut] =
      create_task(callgraph_create, default_config);

  // create call graph
  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(callgraph_task));
  }

  // create type graph

  int nvar = default_config.get(NVAR);
  int nsplits = default_config.get(NSPLITS);
  int nlocs = default_config.get(NLOCS);
  int narith = default_config.get(NARITH);

  // try future get
  auto cg = callgraph_fut.get();

  for (int r_var = 0; r_var < nvar; ++r_var) {
    // create varassign
    // TODO:

    for (int r_splits = 0; r_splits < nsplits; ++r_splits) {
      // create callgraph
      // TODO:

      for (int r_locs = 0; r_locs < nlocs; ++r_locs) {
        // create locIR
        // TODO:

        for (int r_exprs = 0; r_exprs < narith; ++r_exprs) {
          // create exprIR
          // TODO:
        }
      }
    }
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
