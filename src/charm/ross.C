#include "ross.h"
#include "pe.h"

extern CProxy_PE pes;

Initialize::Initialize(CkArgMsg *m) {
  pes = CProxy_PE::ckNew(thisProxy);
  delete m;
}

#include "ross.def.h"
