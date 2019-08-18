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

#include "config/configs.h"
#include "tasksystem.h"
#include "timestamp.h"
#include "version.h"

std::queue<task_t> task_queue;
std::mutex task_queue_mutex;
std::mutex mut_dbgs;

void consumer_thread_func();

using typegraph_future_t =
    decltype(std::packaged_task<tg_task_type>{}.get_future());
using callgraph_future_t =
    decltype(std::packaged_task<cg_task_type>{}.get_future());
using varassign_future_t =
    decltype(std::packaged_task<va_task_type>{}.get_future());
using contgraph_future_t =
    decltype(std::packaged_task<cn_task_type>{}.get_future());

using typegraph_sp_t = decltype(typegraph_future_t{}.get());
using callgraph_sp_t = decltype(callgraph_future_t{}.get());
using varassign_sp_t = decltype(varassign_future_t{}.get());
using contgraph_sp_t = decltype(contgraph_future_t{}.get());

//------------------------------------------------------------------------------
//
// Coerunner: putting main coelacanth tasks into queue in order
//
//------------------------------------------------------------------------------

class coerunner_t {
  std::unique_ptr<cfg::config> default_config_;
  std::vector<std::thread> consumers_;
  typegraph_future_t future_typegraph_;
  typegraph_sp_t typegraph_;
  callgraph_future_t future_callgraph_;
  callgraph_sp_t callgraph_;
  std::vector<varassign_future_t> future_assigns_;
  std::vector<varassign_sp_t> assigns_;
  std::vector<std::vector<contgraph_future_t>> future_contgraphs_;
  std::vector<std::vector<contgraph_sp_t>> contgraphs_;
  int nvar_;
  int nsplits_;
  int nlocs_;
  int narith_;

public:
  coerunner_t() {}
  void run(int argc, char **argv);

private:
  auto decide_tg_task();
  void run_typegraph();
  void wait_typegraph();
  void run_callgraph();
  void run_varassign();
  void run_controlgraph();
  void run_locir();
  void run_exprir();
  void run_seq();
};

// logic is simple: stop-after-something means that. Just stop.
void coerunner_t::run_seq() {
  run_typegraph();
  if (cfg::get(*default_config_, PGC::STOP_ON_TG)) {
    wait_typegraph();
    if (!default_config_->quiet())
      std::cout << "Typegraph done, stopping" << std::endl;
    return;
  }

  run_callgraph();
  if (cfg::get(*default_config_, PGC::STOP_ON_CG))
    return;

  run_varassign();
  if (cfg::get(*default_config_, PGC::STOP_ON_VA))
    return;

  run_controlgraph();
  if (cfg::get(*default_config_, PGC::STOP_ON_CN))
    return;

  run_locir();
  run_exprir();
}

void coerunner_t::run(int argc, char **argv) {
  default_config_ =
      std::make_unique<cfg::config>(cfg::read_global_config(argc, argv));

  if (!default_config_->quiet())
    std::cout << "Coelacanth info: git hash = " << GIT_COMMIT_HASH
              << ", built on " << TIMESTAMP << std::endl;

  if (default_config_->dumps()) {
    std::ofstream of("initial.cfg");
    default_config_->dump(of);
  }

  nvar_ = cfg::get(*default_config_, PG::VAR);
  nsplits_ = cfg::get(*default_config_, PG::SPLITS);
  nlocs_ = cfg::get(*default_config_, PG::LOCS);
  narith_ = cfg::get(*default_config_, PG::ARITH);

  // consumer threads
  auto nthreads = cfg::get(*default_config_, PG::CONSUMERS);

  if (!default_config_->quiet())
    std::cout << "Starting " << nthreads << " consumer threads" << std::endl;

  for (int i = 0; i < nthreads; ++i)
    consumers_.emplace_back(consumer_thread_func);

  // put all task in sequence
  run_seq();

  // put final task
  task_t sentinel{[] { return -1; }};
  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(sentinel));
  }

  for (int i = 0; i < nthreads; ++i)
    consumers_[i].join();

  if (!default_config_->quiet())
    std::cout << "Done" << std::endl;
}

auto coerunner_t::decide_tg_task() {
  if (cfg::get(*default_config_, PGC::USETG)) {
    std::string tgname = cfg::gets(*default_config_, PGC::TGNAME);
    return create_task<tg_read_task_type>(typegraph_read, tgname,
                                          *default_config_);
  }

  int tgseed = default_config_->rand_positive();
  return create_task<tg_task_type>(typegraph_create, tgseed, *default_config_);
}

// put typegraph task
void coerunner_t::run_typegraph() {
  auto &&[typegraph_task, typegraph_fut] = decide_tg_task();

  future_typegraph_ = std::move(typegraph_fut);
  std::lock_guard<std::mutex> lk{task_queue_mutex};
  task_queue.push(std::move(typegraph_task));
}

void coerunner_t::wait_typegraph() {
  typegraph_ = future_typegraph_.get();

  if (default_config_->dumps()) {
    std::ofstream of("initial.types");
    typegraph_dump(typegraph_, of);
  }
}

void coerunner_t::run_callgraph() {
  wait_typegraph();

  int cgseed = default_config_->rand_positive();

  // put callgraph task
  auto &&[callgraph_task, callgraph_fut] = create_task<cg_task_type>(
      callgraph_create, cgseed, *default_config_, typegraph_);

  future_callgraph_ = std::move(callgraph_fut);

  {
    std::lock_guard<std::mutex> lk{task_queue_mutex};
    task_queue.push(std::move(callgraph_task));
  }
}

// put varassign tasks
void coerunner_t::run_varassign() {
  callgraph_ = future_callgraph_.get();

  if (default_config_->dumps()) {
    std::ofstream of("initial.calls");
    callgraph_dump(callgraph_, of);
  }

  for (int r_var = 0; r_var < nvar_; ++r_var) {
    int vaseed = default_config_->rand_positive();
    auto &&[vassign_task, vassign_fut] = create_task<va_task_type>(
        varassign_create, vaseed, *default_config_, typegraph_, callgraph_);

    future_assigns_.emplace_back(std::move(vassign_fut));

    {
      std::lock_guard<std::mutex> lk{task_queue_mutex};
      task_queue.push(std::move(vassign_task));
    }
  }
}

void coerunner_t::run_controlgraph() {
  future_contgraphs_.resize(nvar_);
  contgraphs_.resize(nvar_);
  for (int r_var = 0; r_var < nvar_; ++r_var) {
    auto vassign = future_assigns_[r_var].get();

    if (default_config_->dumps()) {
      std::ostringstream os;
      os << "varassign." << r_var;
      std::ofstream of(os.str());
      varassign_dump(vassign, of);
    }

    for (int r_splits = 0; r_splits < nsplits_; ++r_splits) {
      int cnseed = default_config_->rand_positive();
      auto &&[cn_task, cn_fut] = create_task<cn_task_type>(
          controlgraph_create, cnseed, *default_config_, typegraph_, callgraph_,
          vassign);
      {
        std::lock_guard<std::mutex> lk{task_queue_mutex};
        task_queue.push(std::move(cn_task));
      }
      future_contgraphs_[r_var].emplace_back(std::move(cn_fut));
    }

    assigns_.emplace_back(std::move(vassign));
  }
}

void coerunner_t::run_locir() {
  for (int r_var = 0; r_var < nvar_; ++r_var) {
    for (int r_split = 0; r_split < nsplits_; ++r_split) {
      auto cngraph = future_contgraphs_[r_var][r_split].get();

      if (default_config_->dumps()) {
        std::ostringstream os;
        os << "controlgraph." << r_var << "." << r_split;
        std::ofstream of(os.str());
        controlgraph_dump(cngraph, of);
      }

      for (int r_loc = 0; r_loc < nlocs_; ++r_loc) {
        // TODO: put LocIR tasks, store futures
      }

      contgraphs_[r_var].emplace_back(std::move(cngraph));
    }
  }
}

void coerunner_t::run_exprir() {
  // TODO: put ExprIR tasks
}

//------------------------------------------------------------------------------
//
// consumer_thread_func -- entry point for queue consumer thread
//
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
//
// main programm entry point
//
//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  coerunner_t programm;
  programm.run(argc, argv);
}
