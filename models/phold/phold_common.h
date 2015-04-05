#ifndef INC_phold_h
#define INC_phold_h

	/*
	 * PHOLD Types
	 */

typedef struct phold_state {
  long int  dummy_state;
} phold_state;

typedef struct phold_message {
  long int  is_long;
} phold_message;

	/*
	 * PHOLD Globals
	 */

static unsigned int stagger = 0;
static tw_stime percent_remote = 0.1;
static int start_events = 1;
static tw_stime mean = 1.0;
static int long_start_events = 0;
static tw_stime long_mean = 5.0;

static char run_id[1024] = "undefined";

	/*
	 * PHOLD Functions
	 */

inline tw_stime regular_delay(tw_lp* lp) {
  return tw_rand_exponential(lp->rng, mean);
}

inline tw_stime long_delay(tw_lp* lp) {
  return tw_rand_exponential(lp->rng, long_mean);
}

void
phold_init(phold_state* s, tw_lp* lp) {
  tw_stime offset;
  tw_event* e;

  for (int i = 0; i < start_events; i++) {
    bool is_long = i < long_start_events;

    // Set offset based on if the event is long, and whether we have stagger.
    offset = ROSS_CONSTANT(g_tw_lookahead);
    if (is_long) {
      offset += long_delay(lp);
    } else {
      offset += regular_delay(lp);
    }
    if( stagger ) {
      offset += (tw_stime)(lp->gid % (unsigned int)ROSS_CONSTANT(g_tw_ts_end));
    }

    // Create and send the event, marking whether it is long.
    e = tw_event_new(lp->gid, offset, lp);
    ((phold_message*)(tw_event_data(e)))->is_long = is_long;
    tw_event_send(e);
  }
}

void
phold_event_handler(phold_state* s, tw_bf* bf, phold_message* m, tw_lp* lp) {
  tw_lpid	 dest;

  if(tw_rand_unif(lp->rng) <= percent_remote) {
    bf->c1 = 1;
    dest = tw_rand_integer(lp->rng, 0, ROSS_CONSTANT(g_total_lps) - 1);
  } else {
    bf->c1 = 0;
    dest = lp->gid;
  }

  if(dest < 0 || dest >= ROSS_CONSTANT(g_total_lps)) {
    tw_error(TW_LOC, "bad dest");
  }

  // Set offset based on whether this event is part of a long chain or not.
  tw_stime offset = ROSS_CONSTANT(g_tw_lookahead);
  if (m->is_long) {
    offset += long_delay(lp);
  } else {
    offset += regular_delay(lp);
  }

  tw_event* e = tw_event_new(dest, offset, lp);
  ((phold_message*)(tw_event_data(e)))->is_long = m->is_long;
  tw_event_send(e);
}

void
phold_event_handler_rc(phold_state* s, tw_bf* bf, phold_message* m, tw_lp* lp) {
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if(bf->c1 == 1) {
    tw_rand_reverse_unif(lp->rng);
  }
}

void
phold_finish(phold_state * s, tw_lp * lp) {
}


const tw_optdef app_opt[] =
{
  TWOPT_GROUP("PHOLD Model"),
  TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
  TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
  TWOPT_UINT("start-events", start_events, "number of initial messages per LP"),
  TWOPT_UINT("long-start-events", long_start_events, "number of long initial messages per LP"),
  TWOPT_STIME("long-mean", long_mean, "exponential distribution mean for timestamps of long events"),
  TWOPT_UINT("stagger", stagger, "Set to 1 to stagger event uniformly across 0 to end time."),
  TWOPT_CHAR("run", run_id, "user supplied run name"),
  TWOPT_END()
};

#endif
