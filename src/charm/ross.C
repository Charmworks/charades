#include "ross.h"
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

char** CopyArgs(int argc, char** argv);
void charm_init(int argc, char** argv) {
  char** argv_copy = CopyArgs(argc,argv);
  CharmInit(argc, argv_copy);
}
void charm_exit() {
  CharmLibExit();
}
void charm_run() {
  scheduler_proxy.ckLocalBranch()->start_simulation();
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

#include "ross.def.h"
