#include "charm_setup.h"

#include "scheduler.h"
#include "statistics.h"

CProxy_Initialize mainProxy;

Initialize::Initialize(CkArgMsg *m) {
  delete m;
  mainProxy = thisProxy;
  CkExit();
}

void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
  traceRegisterUserEvent("Forward Execution", USER_EVENT_FWD);
  traceRegisterUserEvent("Rollback", USER_EVENT_RB);
  traceRegisterUserEvent("Cancellation", USER_EVENT_CANCEL);
  traceRegisterUserEvent("GVT", USER_EVENT_GVT);
  traceRegisterUserEvent("LB", USER_EVENT_LDB);
  traceRegisterUserEvent("Fossil Collection", USER_EVENT_FC);
}
void charm_exit() {
  CharmLibExit();
}
void charm_run() {
  ((Scheduler*)CkLocalBranch(scheduler_id))->start_simulation();
  StartCharmScheduler();
}

int tw_ismaster() {
  return (CkMyPe() == 0);
}
int tw_nnodes() {
  return CkNumPes();
}
int tw_mype() {
  return CkMyPe();
}

#include "charm_setup.def.h"
