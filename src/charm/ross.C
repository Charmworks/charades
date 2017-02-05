#include "ross.h"
#include "pe.h"
#include "gvtmanager.h"

extern CProxy_PE pes;
extern CProxy_GvtManager gvts;
CProxy_Initialize mainProxy;

// Function that starts the charm library and results in the creation of the
// PE group chares.
void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
}

Initialize::Initialize(CkArgMsg *m) {
  mainProxy = thisProxy;
  pes = CProxy_PE::ckNew(thisProxy);
  gvts = CProxy_GvtSynch::ckNew(thisProxy);
  delete m;
}

#include "ross.def.h"
