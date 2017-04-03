#ifndef _ROSS_API_H
#define _ROSS_API_H

#include "charm_functions.h"
#include "event.h"
#include "globals.h"
#include "lp.h"
#include "ross_random.h"
#include "ross_clcg4.h"
#include "ross_opts.h"
#include "ross_util.h"
#include "ross_setup.h"
#include "ross_block.h"

#include <charm++.h>
#define g_tw_mynode CkMyPe()
#define g_tw_npe 1

// TODO: Move these
int tw_ismaster();
int tw_nnodes();
tw_stime tw_now(tw_lp*);

#define tw_event_data(e) (e->userData)

#endif
