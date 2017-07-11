/** \file ross_api.h
 *  Single include for models which includes all other required headers.
 *
 *  \todo Get rid of/move the other random declarations and defines as needed.
 *  \todo Rename file for Charades
 */

#ifndef _ROSS_API_H
#define _ROSS_API_H

#include "event.h"
#include "globals.h"
#include "lp.h"
#include "ross_random.h"
#include "ross_clcg4.h"
#include "ross_opts.h"
#include "ross_setup.h"
#include "ross_block.h"
#include "util.h"

#include <charm++.h>

/** Macro for determinig the local node number */
#define g_tw_mynode CkMyPe()
/** \deprecated Macro for determining the local node size */
#define g_tw_npe 1
/** Macro to access the userData for an event as a void* */
#define tw_event_data(e) (e->userData)

int tw_ismaster();        ///< returns true if we are rank 0
int tw_nnodes();          ///< returns the number of nodes in the job
tw_stime tw_now(tw_lp*);  ///< returns the current time of the given lp

#endif
