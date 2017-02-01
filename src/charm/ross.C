#include "ross.h"
#include "scheduler.h"

extern CProxy_Scheduler scheduler;
CProxy_Initialize mainProxy;

// Function that starts the charm library and results in the creation of the
// PE group chares.
void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
}

Initialize::Initialize(CkArgMsg *m) {
  mainProxy = thisProxy;
  scheduler = CProxy_ConservativeScheduler::ckNew(thisProxy);
  delete m;
}

#include "ross.def.h"
