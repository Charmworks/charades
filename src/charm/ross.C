#include "ross.h"
#include "scheduler.h"
#include "gvtmanager.h"
#include "ross_opts.h"
#include "globals.h"

CProxy_Initialize mainProxy;

Initialize::Initialize(CkArgMsg *m) {
  mainProxy = thisProxy;

  // This is temporary to allow selection of scheduler
  static const tw_optdef temp_options[] = {
    // TODO: Make sure all relevant constants can be set from the command line
    TWOPT_GROUP("Temporary Init Opts"),
    TWOPT_UINT("synch", g_tw_synchronization_protocol, "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4"),
    TWOPT_END()
  };
  tw_opt_add(temp_options);
  tw_opt_parse(&m->argc, &m->argv);

  if (g_tw_synchronization_protocol == 1) {
    scheduler_proxy = CProxy_SequentialScheduler::ckNew();
  } else if (g_tw_synchronization_protocol == 2) {
    scheduler_proxy = CProxy_ConservativeScheduler::ckNew();
  } else if (g_tw_synchronization_protocol == 3) {
    scheduler_proxy = CProxy_OptimisticScheduler::ckNew();
  } else {
    CkAbort("Unknown synchronization protocol\n");
  }
  gvt_manager_proxy = CProxy_SyncGVT::ckNew();
  delete m;
}

void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
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
