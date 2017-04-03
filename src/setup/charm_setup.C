#include "charm_setup.h"
#include "scheduler.h"
#include "gvtmanager.h"
#include "ross_opts.h"
#include "globals.h"

CProxy_Initialize mainProxy;

Initialize::Initialize(CkArgMsg *m) {
  delete m;
  mainProxy = thisProxy;
  CkExit();
}

void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
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

void tw_abort(const char* error) {
  CkAbort(error);
}

#include "charm_setup.def.h"
