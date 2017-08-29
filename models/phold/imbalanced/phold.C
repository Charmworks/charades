#include "phold.h"

void PHoldLP::initialize() {
  for (int i = 0; i < start_events; i++) {
    // Determine the offset based on our lps mean delay
    Time offset = g_tw_lookahead + mean_delay * tw_rand_exponential(rng, 1.0);

    // Create and send the event
    Event* e = tw_event_new<PHoldMessage>(gid, offset, this);
    tw_event_send(e);
  }
}

void PHoldLP::forward(PHoldMessage* msg, tw_bf* bf) {
  // First, process the load for the event. The load is in microseconds.
  double start = CmiWallTimer();
  while ((CmiWallTimer()-start) < (work_load + msg->work_load)/1000000);

  // Set destination
  uint64_t dest;
  if (tw_rand_unif(rng) < percent_remote) {
    bf->c1 = 1;
    if (region_size == g_total_lps) {
      dest = tw_rand_integer(rng, 0, g_total_lps-1);
    } else {
      uint64_t doffset = tw_rand_integer(rng, 0, region_size) - (region_size/2);
      dest = (gid + doffset + g_total_lps) % g_total_lps;
    }
  } else {
    bf->c1 = 0;
    dest = gid;
  }

  // Set offset
  Time mean = mean_delay + msg->mean_delay;
  Time offset = g_tw_lookahead + mean * tw_rand_exponential(rng, 1.0);
  Event* e = tw_event_new<PHoldMessage>(dest, offset, this);
  tw_event_send(e);
}

void PHoldLP::reverse(PHoldMessage* msg, tw_bf* bf) {
  // We definitely used rng for offset and remote percent
  tw_rand_reverse_unif(rng);
  tw_rand_reverse_unif(rng);

  // If it was a remote message then we also used rng for the destination
  if (bf->c1 == 1) {
    tw_rand_reverse_unif(rng);
  }
}

void PHoldLP::commit(PHoldMessage* msg, tw_bf* bf) {}
void PHoldLP::finalize() {}

// Every LP in the PHOLD model has the same type.
class PHoldLPFactory : public LPFactory {
  public:
    LPBase* create_lp(uint64_t gid) const {
      return new PHoldLP(lp_load_map(gid), lp_delay_map(gid), lp_remote_map(gid));
    }
};

const tw_optdef app_opt[] =
{
  TWOPT_GROUP("PHOLD Model"),
  TWOPT_UINT("start-events", start_events, "number of initial messages per LP"),

  TWOPT_UINT("load-map", load_map, "0 - Uniform, 1 - Blocked, 2 - Linear"),
  TWOPT_DOUBLE("percent-heavy", percent_heavy, "desired percent of heavy sends [0.0=1.0]"),
  TWOPT_UINT("light-load", light_load, "load for lightly loaded lps in us"),
  TWOPT_UINT("heavy-load", heavy_load, "load for heavily loaded lps in us"),
  TWOPT_UINT("load-seed", load_seed, "extra param used by certain load maps"),

  TWOPT_UINT("delay-map", delay_map, "0 - Uniform"),
  TWOPT_DOUBLE("percent-long", percent_long, "desired percent of long sends [0.0-1.0]"),
  TWOPT_DOUBLE("short-delay", short_delay, "exponential distribution mean for event delays"),
  TWOPT_DOUBLE("long-delay", long_delay, "long exponential distribution mean for event delays"),
  TWOPT_UINT("delay-seed", delay_seed, "used with some delay maps"),

  TWOPT_UINT("remote-map", remote_map, "0 - Uniform, 1 - Blocked"),
  TWOPT_DOUBLE("percent-greedy", percent_greedy, "desired percent of greedy lps [0.0-1.0]"),
  TWOPT_DOUBLE("generous-remote", generous_remote, "remote percent for generous lps [0.0-1.0]"),
  TWOPT_DOUBLE("greedy-remote", greedy_remote, "remote percent for greedy lps [0.0-1.0]"),
  TWOPT_UINT("remote-seed", remote_seed, "extra param used by certain remote maps"),

  TWOPT_UINT("region-size", region_size, "defines the size of the region in which lps send events"),
  TWOPT_END()
};

int main(int argc, char **argv, char **env) {
  // Add the model specific options, then initialize ROSS and Charm++
  tw_opt_add(app_opt);
  tw_init(argc, argv);

  register_msg_type<PHoldMessage>();

  // Check for a valid configuration
  TW_ASSERT(g_tw_lookahead <= 1000, "Bad lookahead value\n");
  TW_ASSERT(region_size <= g_total_lps, "Region size invalid\n");

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
      CkAbort("Bad load map type specified\n");
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
      CkAbort("Bad delay map type specified\n");
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
      CkAbort("Bad remote map type specified\n");
  }

  // Call tw_define_lps to create LPs and event queues
  tw_create_simulation(new PHoldLPFactory());

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
    printf("   Short Delay............%llu\n", short_delay);
    printf("   Long Delay.............%llu\n", long_delay);
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
