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

#include "coelacanth.h"

//------------------------------------------------------------------------------
//
// main programm entry point
//
//------------------------------------------------------------------------------

int main(int argc, char **argv) {
  coerunner_t programm;
  programm.run(argc, argv);
}

//------------------------------------------------------------------------------
//
// Coerunner: putting main coelacanth tasks into queue in order
//
//------------------------------------------------------------------------------

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

  auto nthreads = cfg::get(*default_config_, PG::CONSUMERS);
  if (!default_config_->quiet())
    std::cout << "Starting " << nthreads << " consumer threads" << std::endl;

  for (int i = 0; i < nthreads; ++i)
    consumers_.emplace_back(consumer_thread_func);

  nvar_ = cfg::get(*default_config_, PG::VAR);
  nsplits_ = cfg::get(*default_config_, PG::SPLITS);

  run_typegraph();

  push_sentinel_task();

  for (int i = 0; i < nthreads; ++i)
    consumers_[i].join();

  if (!default_config_->quiet())
    std::cout << "Done" << std::endl;
}

void coerunner_t::run_typegraph() {
  auto &&[typegraph_task, typegraph_fut] = decide_tg_task();

  push_task(std::move(typegraph_task));

  cg_task_req_state_t sub{typegraph_fut.get()};

  if (default_config_->dumps()) {
    std::ofstream of("initial.types");
    typegraph_dump(sub.tg, of);
  }

  if (cfg::get(*default_config_, PGC::STOP_ON_TG)) {
    if (!default_config_->quiet())
      std::cout << "Typegraph done, stopping" << std::endl;
    return;
  }

  run_callgraph(sub);
}

void coerunner_t::run_callgraph(cg_task_req_state_t s) {
  int cgseed = default_config_->rand_positive();

  auto &&[callgraph_task, callgraph_fut] =
      create_task(callgraph_create, cgseed, *default_config_, s.tg);

  push_task(std::move(callgraph_task));

  va_task_req_state_t sub{s, callgraph_fut.get()};

  if (default_config_->dumps()) {
    std::ofstream of("initial.calls");
    callgraph_dump(sub.cg, of);
  }

  if (cfg::get(*default_config_, PGC::STOP_ON_CG)) {
    if (!default_config_->quiet())
      std::cout << "Callgraph done, stopping" << std::endl;
    return;
  }

  run_varassign(sub);
}

void coerunner_t::run_varassign(va_task_req_state_t s) {
  std::vector<varassign_future_t> future_assigns;
  future_assigns.reserve(nvar_);

  for (int i = 0; i < nvar_; ++i) {
    int vaseed = default_config_->rand_positive();
    auto &&[vassign_task, vassign_fut] =
        create_task(varassign_create, vaseed, *default_config_, s.tg, s.cg);

    future_assigns.emplace_back(std::move(vassign_fut));
    push_task(std::move(vassign_task));
  }

  auto stop_after_va = cfg::get(*default_config_, PGC::STOP_ON_VA);

  for (int i = 0; i < nvar_; ++i) {
    cn_task_req_state_t sub{s, future_assigns[i].get(), i};
    if (default_config_->dumps()) {
      std::ostringstream os;
      os << "varassign." << i;
      std::ofstream of(os.str());
      varassign_dump(sub.va, of);
    }

    if (!stop_after_va)
      run_controlgraph(sub);
  }
}

void coerunner_t::run_controlgraph(cn_task_req_state_t s) {
  std::vector<contgraph_future_t> future_contgraphs;
  future_contgraphs.reserve(nsplits_);

  for (int i = 0; i < nsplits_; ++i) {
    int cnseed = default_config_->rand_positive();
    auto &&[cn_task, cn_fut] = create_task(controlgraph_create, cnseed,
                                           *default_config_, s.tg, s.cg, s.va);
    future_contgraphs.emplace_back(std::move(cn_fut));
    push_task(std::move(cn_task));
  }

  auto stop_after_cn = cfg::get(*default_config_, PGC::STOP_ON_CN);

  for (int i = 0; i < nsplits_; ++i) {
    li_task_req_state_t sub{s, future_contgraphs[i].get(), i};
    if (default_config_->dumps()) {
      std::ostringstream os;
      os << "controlgraph." << s.nva << "." << i;
      std::ofstream of(os.str());
      controlgraph_dump(sub.cn, of);
    }

    if (!stop_after_cn)
      run_locir(sub);
  }
}

void coerunner_t::run_locir(li_task_req_state_t s) {
  // TODO: put locir tasks
  ei_task_req_state_t sub{s};
  run_exprir(sub);
}

void coerunner_t::run_exprir(ei_task_req_state_t) {
  // TODO: put exprir tasks
}
