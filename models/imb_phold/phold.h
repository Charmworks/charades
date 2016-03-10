#ifndef INC_phold_h
#define INC_phold_h

#include "ross_api.h"

typedef struct phold_state {
  int dummy_state;
} phold_state;

typedef struct phold_message {
  int work_load;
  tw_stime mean_delay;
} phold_message;

	/*
	 * PHOLD Globals
	 */
static int start_events = 1;
static tw_stime percent_remote = 0.1;

static tw_stime percent_heavy = 0.0;
static int regular_load = 0;
static int heavy_load = 0;
static int heavy_seed = 0;

static tw_stime percent_long = 0.0;
static tw_stime regular_mean = 1.0;
static tw_stime long_mean = 5.0;

#endif
