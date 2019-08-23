/** \file setup.h
 * Contains setup routines called from main to initialize, run, and terminate
 * a simulation.
 */
#ifndef _CHARM_SETUP_H
#define _CHARM_SETUP_H

#include "setup.decl.h"

#include "event.h"
#include "lp.h"
#include "mpi-interoperate.h" // This has to be included here

/** Parses commandline arguments and initializes the simulator */
void tw_init(int argc, char** argv);
/** Creates simulation components based on desired mapping */
void tw_create_simulation(LPFactory* factory, LPMapper* mapper);
/** Creates simulation components using default mapping */
void tw_create_simulation(LPFactory* factory);
/** Intializes LPs then runs simulation to completion */
void tw_run();
/** Cleans up simulation resources and exits */
void tw_end();

void charm_init(int argc, char** argv);
void charm_exit();
void charm_run();

#endif
