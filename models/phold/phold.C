#include "phold.h"

void
phold_init(phold_state* s, tw_lp* lp) {
  tw_stime offset;

  for (int i = 0; i < start_events; i++) {
    offset = tw_rand_exponential(lp->rng, mean) + ROSS_CONSTANT(g_tw_lookahead);
    if( stagger ) {
      offset += (tw_stime)(lp->gid % (unsigned int)ROSS_CONSTANT(g_tw_ts_end));
    }
    tw_event_send(tw_event_new(lp->gid, offset, lp));
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

  tw_event_send(
      tw_event_new(
            dest,
            tw_rand_exponential(lp->rng, mean) + ROSS_CONSTANT(g_tw_lookahead),
            lp));
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

tw_lptype mylps[] = {
  { (init_f) phold_init,
    (event_f) phold_event_handler,
    (revent_f) phold_event_handler_rc,
    (final_f) phold_finish,
    sizeof(phold_state) },
  {0},
};

// Every LP in the PHOLD model has the same type.
tw_lptype* phold_type_map(tw_lpid global_id) {
  return &mylps[0];
}

const tw_optdef app_opt[] =
{
  TWOPT_GROUP("PHOLD Model"),
  TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
  TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
  TWOPT_UINT("start-events", start_events, "number of initial messages per LP"),
  TWOPT_UINT("stagger", stagger, "Set to 1 to stagger event uniformly across 0 to end time."),
  TWOPT_CHAR("run", run_id, "user supplied run name"),
  TWOPT_END()
};

int main(int argc, char **argv, char **env) {
  // Add the model specific options, then initialize ROSS and Charm++
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  // Check for a valid lookahead, and set the mean based on lookahead
  if( ROSS_CONSTANT(g_tw_lookahead) > 1.0 ) {
    tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
  }
  mean = mean - ROSS_CONSTANT(g_tw_lookahead);

  // Set constants for char creation and maps
  ROSS_CONSTANT(g_type_map) = phold_type_map;

  // Call tw_define_lps to create LPs and event queues
  tw_define_lps(sizeof(phold_message), 0);

  if(tw_ismaster()) {
    printf("========================================\n");
    printf("PHOLD Model Configuration..............\n");
    printf("   Start-events...........%u\n", start_events);
    printf("   stagger................%u\n", stagger);
    printf("   Mean...................%lf\n", mean);
    printf("   Remote.................%lf\n", percent_remote);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();

  return 0;
}
