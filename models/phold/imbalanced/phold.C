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
  rng.prev();
  rng.prev();

  // If it was a remote message then we also used rng for the destination
  if (bf->c1 == 1) {
    rng.prev();
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


int main(int argc, char **argv, char **env) {
  ArgumentSet args("PHOLD Model");
  args.register_argument("start-events", "number of initial messages per LP", start_events);
  args.register_argument("load-map", "0 - Uniform, 1 - Blocked, 2 - Linear", load_map);
  args.register_argument("percent-heavy", "desired percent of heavy sends [0.0=1.0]", percent_heavy);
  args.register_argument("light-load", "load for lightly loaded lps in us", light_load);
  args.register_argument("heavy-load", "load for heavily loaded lps in us", heavy_load);
  args.register_argument("load-seed", "extra param used by certain load maps", load_seed);
  args.register_argument("delay-map", "0 - Uniform", delay_map);
  args.register_argument("percent-long", "desired percent of long sends [0.0-1.0]", percent_long);
  args.register_argument("short-delay", "exponential distribution mean for event delays", short_delay);
  args.register_argument("long-delay", "long exponential distribution mean for event delays", long_delay);
  args.register_argument("delay-seed", "used with some delay maps", delay_seed);
  args.register_argument("remote-map", "0 - Uniform, 1 - Blocked", remote_map);
  args.register_argument("percent-greedy", "desired percent of greedy lps [0.0-1.0]", percent_greedy);
  args.register_argument("generous-remote", "remote percent for generous lps [0.0-1.0]", generous_remote);
  args.register_argument("greedy-remote", "remote percent for greedy lps [0.0-1.0]", greedy_remote);
  args.register_argument("remote-seed", "extra param used by certain remote maps", remote_seed);
  args.register_argument("region-size", "defines the size of the region in which lps send events", region_size);
  tw_add_arguments(&args);

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
