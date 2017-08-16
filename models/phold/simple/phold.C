#include "phold.h"

void PHoldLP::initialize() {
  for (int i = 0; i < start_events; i++) {
    // Set offset using an exponential distribution, adding stagger if necessary
    Time offset = g_tw_lookahead + mean * tw_rand_exponential(rng, 1.0);

    // Create and send the event.
    Event* e = tw_event_new<PHoldMessage>(gid, offset, this);
    tw_event_send(e);
  }
}

void PHoldLP::forward(PHoldMessage* msg, tw_bf* bf) {
  uint64_t dest = gid;
  if(tw_rand_unif(rng) <= percent_remote) {
    bf->c1 = 1;
    dest = tw_rand_integer(rng, 0, g_total_lps - 1);
  }

  Time offset = g_tw_lookahead + tw_rand_exponential(rng, mean);
  Event* e = tw_event_new<PHoldMessage>(dest, offset, this);
  tw_event_send(e);
}

void PHoldLP::reverse(PHoldMessage* msg, tw_bf* bf) {
  // We definitely used rng for offset and remote percent
  tw_rand_reverse_unif(rng);
  tw_rand_reverse_unif(rng);

  // If it was a remote message then we also used rng for the destination
  if(bf->c1 == 1) {
    tw_rand_reverse_unif(rng);
  }
}

void PHoldLP::commit(PHoldMessage* msg, tw_bf* bf) {}
void PHoldLP::finalize() {}

// Every LP in the PHOLD model has the same type.
LPBase* phold_type_map(uint64_t gid) {
  return new PHoldLP();
}

const tw_optdef app_opt[] =
{
  TWOPT_GROUP("PHOLD Model"),
  TWOPT_DOUBLE("remote", percent_remote, "desired remote event rate"),
  TWOPT_UINT("mean", mean, "exponential distribution mean for timestamps"),
  TWOPT_UINT("start-events", start_events, "number of initial messages per LP"),
  TWOPT_END()
};

int main(int argc, char **argv, char **env) {
  // Add the model specific options, then initialize ROSS and Charm++
  tw_opt_add(app_opt);
  tw_init(argc, argv);

  register_msg_type<PHoldMessage>();

  // Check for a valid configuration
  if (g_tw_lookahead > 1000) {
    CkAbort("Lookahead > 1000 .. needs to be less\n");
  }

  // Adjust mean based on lookahead
  mean = mean - g_tw_lookahead;

  // Type map must be set before tw_define_lps, all other maps will be default
  g_type_map = phold_type_map;

  // Call tw_define_lps to create LPs and event queues
  tw_create_lps();

  if (tw_ismaster()) {
    printf("========================================\n");
    printf("PHOLD Model Configuration..............\n");
    printf("   Start events...........%u\n", start_events);
    printf("   Mean...................%llu\n", mean);
    printf("   Remote.................%lf\n", percent_remote);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();

  return 0;
}
