#include "phold.h"

inline tw_stime regular_delay(tw_lp* lp) {
  return tw_rand_exponential(lp->rng, regular_mean);
}

inline tw_stime long_delay(tw_lp* lp) {
  return tw_rand_exponential(lp->rng, long_mean);
}

inline int get_load(tw_lpid dest) {
  if (heavy_seed == 0 || dest == 0 || dest % heavy_seed) {
    return regular_load;
  } else {
    return heavy_load;
  }
}

void
phold_init(phold_state* s, tw_lp* lp) {
  tw_stime offset;
  tw_event* e;

  for (int i = 0; i < start_events; i++) {
    // Set offset based on if the event is long, and whether we have stagger.
    offset = g_tw_lookahead;
    if (tw_rand_unif(lp->rng) < percent_long) {
      offset += long_delay(lp);
    } else {
      offset += regular_delay(lp);
    }

    // Create and send the event, marking whether it is long.
    e = tw_event_new(lp->gid, offset, lp);
    ((phold_message*)(e->userData))->work_load = get_load(lp->gid);
    tw_event_send(e);
  }
}

void
phold_event_handler(phold_state* s, tw_bf* bf, phold_message* m, tw_lp* lp) {
  tw_lpid	 rand_dest, dest;
  tw_stime remote_val, heavy_val, long_val, offset;

  // First, process the message load
  for (int i = 0; i < m->work_load; i++) {
    s->dummy_state += i * m->work_load;
  }

  // Then send a new message
  remote_val = tw_rand_unif(lp->rng);
  heavy_val = tw_rand_unif(lp->rng);
  long_val = tw_rand_unif(lp->rng);

  // Set destination
  rand_dest = tw_rand_integer(lp->rng, 0, g_total_lps-1);
  if (remote_val < percent_remote) {
    dest = rand_dest;
    if (heavy_seed && heavy_val < percent_heavy) {
      dest = (dest / heavy_seed) * heavy_seed;
    }
  } else {
    dest = lp->gid;
  }

  // Set offset
  offset = g_tw_lookahead;
  if (long_val < percent_long) {
    offset += long_delay(lp);
  } else {
    offset += regular_delay(lp);
  }


  tw_event* e = tw_event_new(dest, offset, lp);
  ((phold_message*)(e->userData))->work_load = get_load(dest);
  tw_event_send(e);
}

void
phold_event_handler_rc(phold_state* s, tw_bf* bf, phold_message* m, tw_lp* lp) {
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
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
  TWOPT_UINT("start_events", start_events, "number of initial messages per LP"),
  TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
  TWOPT_STIME("heavy", percent_heavy, "desired percent of heavy sends"),
  TWOPT_UINT("load", regular_load, "load in ms for events"),
  TWOPT_UINT("heavy_load", heavy_load, "exponential distribution mean for timestamps of long events"),
  TWOPT_UINT("heavy_seed", heavy_seed, "a heavy lp every 'heavy_seed' lps"),
  TWOPT_STIME("long", percent_long, "desired percent of long sends"),
  TWOPT_STIME("mean", regular_mean, "exponential distribution mean for timestamps"),
  TWOPT_STIME("long_mean", long_mean, "exponential distribution mean for timestamps of long events"),
  TWOPT_END()
};

int main(int argc, char **argv, char **env) {
  // Add the model specific options, then initialize ROSS and Charm++
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  // Check for a valid configuration
  if (g_tw_lookahead > 1.0) {
    tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
  }

  // Adjust means based on lookahead
  regular_mean = regular_mean - g_tw_lookahead;
  long_mean = long_mean - g_tw_lookahead;

  // Type map must be set before tw_define_lps, all other maps will be default
  g_type_map = phold_type_map;

  // Call tw_define_lps to create LPs and event queues
  tw_define_lps(sizeof(phold_message), 0);

  if (tw_ismaster()) {
    printf("========================================\n");
    printf("PHOLD Model Configuration..............\n");
    printf("   Start events...........%u\n", start_events);
    printf("   %% Remote..............%lf\n", percent_remote);
    printf("\n");
    printf("   %% Heavy...............%lf\n", percent_heavy);
    printf("   Regular Load...........%u\n", regular_load);
    printf("   Heavy Load.............%u\n", heavy_load);
    printf("   Heavy Seed.............%u\n", heavy_seed);
    printf("\n");
    printf("   %% Long................%lf\n", percent_long);
    printf("   Regular Mean...........%lf\n", regular_mean);
    printf("   Long Mean..............%lf\n", long_mean);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();

  return 0;
}
