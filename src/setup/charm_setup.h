#ifndef _ROSS_H
#define _ROSS_H
// Main header to be included by those using the library

#include "lp.h"
#include "event.h"
#include "charm_setup.decl.h"
#include "mpi-interoperate.h"
#include "charm_functions.h"

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
