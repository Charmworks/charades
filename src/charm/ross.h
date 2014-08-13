#ifndef _ROSS_H
#define _ROSS_H
// Main header to be included by those using the library

#include "lp.h"
#include "event.h"
#include "ross.decl.h"

class Initialize : public CBase_Initialize {
  public:
    Initialize(CkArgMsg *m);

    Initialize(CkMigrateMsg* m) { }

    Exit() {
      CkExit();
    }
};

#endif
