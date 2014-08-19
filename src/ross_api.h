#include "globals.h"

#include "ross_event/ross_event.h"
#include "ross_rand/ross_random.h"
#include "ross_rand/ross_clcg4.h"
#include "ross_opts/ross_opts.h"
#include "ross_util/ross_util.h"
#include "ross_setup/ross_setup.h"
#include "lp_struct.h"

#include <charm++.h>
#define tw_nnodes CkNumPes
#define g_tw_mynode CkMyPe()
#define g_tw_npe 1
