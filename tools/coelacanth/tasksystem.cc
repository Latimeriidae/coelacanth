//------------------------------------------------------------------------------
//
// tasksystem.cc -- task system for coelacanth test generator
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "tasksystem.h"

//------------------------------------------------------------------------------
//
// global task queue support
//
//------------------------------------------------------------------------------

std::queue<task_t> task_queue;
std::mutex task_queue_mutex;

void push_task(task_t tsk) {
  std::lock_guard<std::mutex> lk{task_queue_mutex};
  task_queue.push(std::move(tsk));
}

void push_sentinel_task() {
  task_t sentinel{[] { return -1; }};
  push_task(std::move(sentinel));
}

//------------------------------------------------------------------------------
//
// consumer_thread_func -- entry point for queue consumer thread
//
//------------------------------------------------------------------------------

void consumer_thread_func() {
  task_t cur;
  for (;;) {
    {
      std::lock_guard<std::mutex> lk{task_queue_mutex};  
      if (task_queue.empty()) {
        std::this_thread::yield();
        continue;
      }
      cur = std::move(task_queue.front());
      task_queue.pop();
    }

    int res = std::move(cur)();
    if (res == -1) {
      push_sentinel_task();
      return;
    }
  }
}
