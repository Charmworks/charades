#include "ross.h"
#include "scheduler.h"
#include "gvtmanager.h"
#include "pe.h"
#include "ross_opts.h"

CProxy_Initialize mainProxy;

// Function that starts the charm library and results in the creation of the
// PE group chares.
void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
}

Initialize::Initialize(CkArgMsg *m) {
  mainProxy = thisProxy;

  pe_manager_proxy = CProxy_PEManager::ckNew();
  gvt_manager_proxy = CProxy_SyncGVT::ckNew();

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
  delete m;
}

#include "ross.def.h"
