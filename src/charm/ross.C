#include "NDMeshStreamer.h"
#include "event.h"
#include "ross.h"
#include "pe.h"

extern CProxy_PE pes;

// Function that starts the charm library and results in the creation of the
// PE group chares.
void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
}

Initialize::Initialize(CkArgMsg *m) {
  pes = CProxy_PE::ckNew(thisProxy);
  delete m;
}

#include "ross.def.h"
