#include "ross.h"
#include "scheduler.h"
#include "gvtmanager.h"
#include "pe.h"

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

  // TODO: Hard-coded for now
  g_tw_synchronization_protocol = 3;
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
