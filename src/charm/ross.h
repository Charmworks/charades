#ifndef _ROSS_H
#define _ROSS_H
// Main header to be included by those using the library

#include "lp.h"
#include "event.h"
#include "ross.decl.h"
#include "mpi-interoperate.h"

#define DEBUG(format, ...) { }
#define DEBUG2(format, ...) { }
#define DEBUG3(format, ...) { }
#define DEBUG4(format, ...) { }
//#define DEBUG(format, ...) { CkPrintf(format, ## __VA_ARGS__); }
//#define DEBUG2(format, ...) { CkPrintf(format, ## __VA_ARGS__); }
//#define DEBUG3(format, ...) { CkPrintf(format, ## __VA_ARGS__); }
//#define DEBUG4(format, ...) { CkPrintf(format, ## __VA_ARGS__); }

class Initialize : public CBase_Initialize {
  public:
    Initialize(CkArgMsg *m);

    Initialize(CkMigrateMessage* m) { }

    void Exit() {
      DEBUG("Exit called\n");
      CkExit();
    }
};

#endif
