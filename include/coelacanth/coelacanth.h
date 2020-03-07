//------------------------------------------------------------------------------
//
// coelacanth.h -- main driver for coelacanth test generator, header
//
//------------------------------------------------------------------------------

#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

#include "config/configs.h"
#include "dbgstream.h"
#include "tasksystem.h"
#include "timestamp.h"
#include "version.h"

struct cg_task_req_state_t {
  typegraph_sp_t tg;
  cg_task_req_state_t(typegraph_sp_t t) : tg{t} {}
};

struct va_task_req_state_t : cg_task_req_state_t {
  callgraph_sp_t cg;
  va_task_req_state_t(cg_task_req_state_t p, callgraph_sp_t c)
      : cg_task_req_state_t{p}, cg{c} {}
};

struct cn_task_req_state_t : va_task_req_state_t {
  varassign_sp_t va;
  int nva;
  cn_task_req_state_t(va_task_req_state_t p, varassign_sp_t c, int nv)
      : va_task_req_state_t{p}, va{c}, nva{nv} {}
};

struct li_task_req_state_t : cn_task_req_state_t {
  contgraph_sp_t cn;
  int nc;
  li_task_req_state_t(cn_task_req_state_t p, contgraph_sp_t c, int n)
      : cn_task_req_state_t{p}, cn{c}, nc{n} {}
};

struct ei_task_req_state_t : li_task_req_state_t {
  // TODO
  ei_task_req_state_t(li_task_req_state_t p) : li_task_req_state_t{p} {}
};

class coerunner_t {
  std::unique_ptr<cfg::config> default_config_;
  std::vector<std::thread> consumers_;
  int nvar_;
  int nsplits_;

public:
  coerunner_t() {}
  void run(int argc, char **argv);

private:
  void run_typegraph();
  void run_callgraph(cg_task_req_state_t);
  void run_varassign(va_task_req_state_t);
  void run_controlgraph(cn_task_req_state_t);
  void run_locir(li_task_req_state_t);
  void run_exprir(ei_task_req_state_t);

private:
  auto decide_tg_task() {
    if (cfg::get(*default_config_, PGC::USETG)) {
      std::string tgname = cfg::gets(*default_config_, PGC::TGNAME);
      return create_task(typegraph_read, tgname, *default_config_);
    }

    int tgseed = default_config_->rand_positive();
    return create_task(typegraph_create, tgseed, *default_config_);
  }
};
