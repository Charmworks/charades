#include "phold.h"

void
phold_init(phold_state* s, tw_lp* lp) {
  tw_stime offset;
  tw_event* e;

  // Set my load and mean delays
  s->work_load = lp_load_map(lp->gid);
  s->mean_delay = lp_delay_map(lp->gid);
  s->percent_remote = lp_remote_map(lp->gid);

  for (int i = 0; i < start_events; i++) {
    // Determine the offset based on our lps mean delay
    offset = g_tw_lookahead + tw_rand_exponential(lp->rng, s->mean_delay);

    // Create and send the event
    e = tw_event_new(lp->gid, offset, lp);
    phold_message* msg = (phold_message*)tw_event_data(e);
    msg->work_load = 0;
    msg->mean_delay = 0.0;
    msg->percent_remote = 0.0;
    tw_event_send(e);
  }
}

void
phold_event_handler(phold_state* s, tw_bf* bf, phold_message* m, tw_lp* lp) {
  tw_lpid dest, dest_offset;
  tw_stime offset;

  // First, process the load for the event. The load is in microseconds.
  double start = CmiWallTimer();
  while ((CmiWallTimer()-start) < (s->work_load + m->work_load)/1000000);

  // Set destination
  if (tw_rand_unif(lp->rng) < s->percent_remote) {
    bf->c1 = 1;
    if (region_size == g_total_lps) {
      dest = tw_rand_integer(lp->rng, 0, g_total_lps-1);
    } else {
      dest_offset = tw_rand_integer(lp->rng, 0, region_size) - (region_size/2);
      dest = (lp->gid + dest_offset + g_total_lps) % g_total_lps;
    }
  } else {
    bf->c1 = 0;
    dest = lp->gid;
  }

  // Set offset
  tw_stime mean = s->mean_delay + m->mean_delay;
  offset = g_tw_lookahead + tw_rand_exponential(lp->rng, mean);

  tw_event* e = tw_event_new(dest, offset, lp);
  phold_message* msg = (phold_message*)tw_event_data(e);
  msg->work_load = 0;
  msg->mean_delay = 0.0;
  msg->percent_remote = 0.0;
  tw_event_send(e);
}

void
phold_event_handler_rc(phold_state* s, tw_bf* bf, phold_message* m, tw_lp* lp) {
  // We definitely used rng for offset and remote percent
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  // If it was a remote message then we also used rng for the destination
  if (bf->c1 == 1) {
    tw_rand_reverse_unif(lp->rng);
  }
}

void
phold_finish(phold_state * s, tw_lp * lp) {}

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
  TWOPT_UINT("start-events", start_events, "number of initial messages per LP"),

  TWOPT_UINT("load-map", load_map, "0 - Uniform, 1 - Blocked, 2 - Linear"),
  TWOPT_STIME("percent-heavy", percent_heavy, "desired percent of heavy sends [0.0=1.0]"),
  TWOPT_UINT("light-load", light_load, "load for lightly loaded lps in us"),
  TWOPT_UINT("heavy-load", heavy_load, "load for heavily loaded lps in us"),
  TWOPT_UINT("load-seed", load_seed, "extra param used by certain load maps"),

  TWOPT_UINT("delay-map", delay_map, "0 - Uniform"),
  TWOPT_STIME("percent-long", percent_long, "desired percent of long sends [0.0-1.0]"),
  TWOPT_STIME("short-delay", short_delay, "exponential distribution mean for event delays"),
  TWOPT_STIME("long-delay", long_delay, "long exponential distribution mean for event delays"),
  TWOPT_UINT("delay-seed", delay_seed, "used with some delay maps"),

  TWOPT_UINT("remote-map", remote_map, "0 - Uniform, 1 - Blocked"),
  TWOPT_STIME("percent-greedy", percent_greedy, "desired percent of greedy lps [0.0-1.0]"),
  TWOPT_STIME("generous-remote", generous_remote, "remote percent for generous lps [0.0-1.0]"),
  TWOPT_STIME("greedy-remote", greedy_remote, "remote percent for greedy lps [0.0-1.0]"),
  TWOPT_UINT("remote-seed", remote_seed, "extra param used by certain remote maps"),

  TWOPT_UINT("region-size", region_size, "defines the size of the region in which lps send events"),
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

  if (region_size > g_total_lps) {
    tw_error(TW_LOC, "Region size is larger than total lps");
  }

  if (region_size == 0) {
    region_size = g_total_lps;
  }

  // Set the load map for lps
  switch (load_map) {
    case 0:
      lp_load_map = &uniform_lp_load;
      break;
    case 1:
      lp_load_map = &blocked_lp_load;
      break;
    case 2:
      lp_load_map = &linear_lp_load;
      break;
    default:
      tw_error(TW_LOC, "Bad map type specified\n");
  }

  // Adjust means based on lookahead and set delay map for lps
  short_delay = short_delay - g_tw_lookahead;
  long_delay = long_delay - g_tw_lookahead;

  // Set the delay map for lps
  switch (delay_map) {
    case 0:
      lp_delay_map = &uniform_lp_delay;
      break;
    case 1:
      lp_delay_map = &blocked_lp_delay;
      break;
    case 2:
      lp_delay_map = inverse_blocked_lp_delay;
      break;
    default:
      tw_error(TW_LOC, "Bad map type specified\n");
  }

  // Set the remote map for lps
  switch (remote_map) {
    case 0:
      lp_remote_map = &uniform_lp_remote;
      break;
    case 1:
      lp_remote_map = &blocked_lp_remote;
      break;
    case 2:
      lp_remote_map = &inverse_blocked_lp_remote;
      break;
    default:
      tw_error(TW_LOC, "Bad map type specified\n");
  }

  // Type map must be set before tw_define_lps, all other maps will be default
  g_type_map = phold_type_map;

  // Call tw_define_lps to create LPs and event queues
  tw_define_lps(sizeof(phold_message), 0);

  if (tw_ismaster()) {
    printf("========================================\n");
    printf("PHOLD Model Configuration..............\n");
    printf("   Start events...........%u\n", start_events);
    printf("\n");
    switch (load_map) {
      case 0:
        printf("   Load Map...............UNIFORM\n");
        break;
      case 1:
        printf("   Load Map...............BLOCKED\n");
        break;
      case 2:
        printf("   Load Map...............LINEAR\n");
        break;
    }
    printf("   %% Heavy................%0.2f%%\n", percent_heavy * 100);
    printf("   Light Load.............%i us\n", light_load);
    printf("   Heavy Load.............%i us\n", heavy_load);
    printf("   Heavy Seed.............%u\n", load_seed);
    printf("\n");
    switch (delay_map) {
      case 0:
        printf("   Delay Map..............UNIFORM\n");
        break;
      case 1:
        printf("   Delay Map..............BLOCKED\n");
        break;
      case 2:
        printf("   Delay Map..............INVERSE BLOCKED\n");
        break;
    }
    printf("   %% Long.................%0.2f%%\n", percent_long * 100);
    printf("   Short Delay............%lf\n", short_delay);
    printf("   Long Delay.............%lf\n", long_delay);
    printf("   Delay Seed.............%u\n", delay_seed);
    printf("\n");
    switch (remote_map) {
      case 0:
        printf("   Remote Map.............UNIFORM\n");
        break;
      case 1:
        printf("   Remote Map.............BLOCKED\n");
        break;
      case 2:
        printf("   Remote Map.............INVERSE BLOCKED\n");
        break;
    }
    printf("   %% Greedy...............%0.2f%%\n", percent_greedy * 100);
    printf("   Generous Remote........%0.2f%%\n", generous_remote * 100);
    printf("   Greedy Remote..........%0.2f%%\n", greedy_remote * 100);
    printf("   Remote Seed............%u\n", remote_seed);
    printf("\n");
    printf("   Region Size............%u\n", region_size);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();

  return 0;
}
