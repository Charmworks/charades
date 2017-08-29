#ifndef _CHARM_SETUP_H
#define _CHARM_SETUP_H

#include "charm_setup.decl.h"

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

void tw_init(int argc, char** argv);
void tw_create_simulation(LPFactory* factory, LPMapper* mapper);
void tw_create_simulation(LPFactory* factory);
void tw_run();
void tw_end();

void charm_init(int argc, char** argv);
void charm_exit();
void charm_run();

#endif
