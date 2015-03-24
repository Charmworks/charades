#ifndef _ROSS_CORE_H
#define _ROSS_CORE_H
// Main header to be included by those using the library

#include "lp.h"
#include "event.h"
#include "ross_core.decl.h"
#include "mpi-interoperate.h"
#include "charm_functions.h"

class Initialize : public CBase_Initialize {
  public:
    Initialize(CkArgMsg *m);

    Initialize(CkMigrateMessage* m) { }

    void Exit() {
      CkExit();
    }
};

#endif
