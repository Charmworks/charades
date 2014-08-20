#ifndef _ROSS_API_H
#define _ROSS_API_H

#include "globals.h"

#include "ross_event/ross_event.h"
#include "ross_rand/ross_random.h"
#include "ross_rand/ross_clcg4.h"
#include "ross_opts/ross_opts.h"
#include "ross_util/ross_util.h"
#include "ross_setup/ross_setup.h"
#include "ross_maps/ross_block.h"
#include "lp_struct.h"

#include <charm++.h>
#define tw_nnodes CkNumPes
#define g_tw_mynode CkMyPe()
#define g_tw_npe 1

int tw_ismaster();

#endif
