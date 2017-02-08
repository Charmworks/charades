#include "ross.h"
#include "scheduler.h"
#include "gvtmanager.h"

extern CProxy_Scheduler scheduler;
extern CProxy_GvtManager gvts;
CProxy_Initialize mainProxy;

// Function that starts the charm library and results in the creation of the
// PE group chares.
void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
}

Initialize::Initialize(CkArgMsg *m) {
  mainProxy = thisProxy;
  gvts = CProxy_GvtSync::ckNew();

  // TODO: Hard-coded for now
  g_tw_synchronization_protocol = 3;
  if (g_tw_synchronization_protocol == 1) {
    scheduler = CProxy_SequentialScheduler::ckNew();
  } else if (g_tw_synchronization_protocol == 2) {
    scheduler = CProxy_ConservativeScheduler::ckNew();
  } else if (g_tw_synchronization_protocol == 3) {
    scheduler = CProxy_OptimisticScheduler::ckNew();
  } else {
    CkAbort("Unknown synchronization protocol\n");
  }
  delete m;
}

#include "ross.def.h"
