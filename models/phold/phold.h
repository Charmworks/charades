#ifndef INC_phold_h
#define INC_phold_h

#include "ross_api.h"

	/*
	 * PHOLD Types
	 */

typedef struct phold_state phold_state;
typedef struct phold_message phold_message;

struct phold_state
{
	long int	 dummy_state;
};

struct phold_message
{
	long int	 dummy_data;
};

	/*
	 * PHOLD Globals
	 */
static unsigned int stagger = 0;
static tw_stime percent_remote = 0.1;
static int start_events = 1;
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
