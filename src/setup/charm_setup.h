#ifndef _CHARM_SETUP_H
#define _CHARM_SETUP_H

#include "charm_setup.decl.h"

#include "charm_functions.h"
#include "event.h"
#include "lp.h"
#include "mpi-interoperate.h" // This has to be included here

extern CProxy_Initialize mainProxy;

class Initialize : public CBase_Initialize {
  public:
    Initialize(CkArgMsg *m);

    Initialize(CkMigrateMessage* m) { }

    void Exit() {
      CkPrintf("Exiting via mainchare\n");
      CkExit();
    }
};

#endif
