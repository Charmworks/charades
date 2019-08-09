#ifndef _CHARM_SETUP_H
#define _CHARM_SETUP_H

#include "setup.decl.h"

#include "event.h"
#include "lp.h"
#include "mpi-interoperate.h" // This has to be included here

void tw_init(int argc, char** argv);
void tw_create_simulation(LPFactory* factory, LPMapper* mapper);
void tw_create_simulation(LPFactory* factory);
void tw_run();
void tw_end();

void charm_init(int argc, char** argv);
void charm_exit();
void charm_run();

#endif
