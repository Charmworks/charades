/**
 * \file example.C
 * Definitions for a simple example model.
 * The main components for defining a model are define the LP handlers and
 * types, and initializing the model simulation in the main function.
 */

#include "example.h"

Time g_initial_events;  ///< Number of events sent out during initialization
Time g_mean_delay;      ///< Mean delay in virtual time of new events
Time g_adjusted_mean;   ///< Mean value for RNG, adjusted for lookahead

void ExampleLP::initialize() {
  counter = 0;
  neighbor = (gid + 1) % g_total_lps;

  for (int i = 0; i < g_initial_events; i++) {
    Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
    Event* e = tw_event_new(neighbor, delay, this);
    ExampleMessage* msg = reinterpret_cast<ExampleMessage*>(e->userData());
    msg->payload = counter++;
    tw_event_send(e);
  }
}

void ExampleLP::forward(ExampleMessage* msg, tw_bf* bf) {
  Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
  Event* e = tw_event_new(neighbor, delay, this);
  ExampleMessage* new_msg = reinterpret_cast<ExampleMessage*>(e->userData());
  new_msg->payload = msg->payload + counter++;
  tw_event_send(e);
}

void ExampleLP::reverse(ExampleMessage* msg, tw_bf* bf) {
  tw_rand_reverse_unif(rng);
  counter--;
}

void ExampleLP::commit(ExampleMessage* msg, tw_bf* bf) {}
void ExampleLP::finalize() {}

/** LP Factory */
LPBase* example_type_map(uint64_t gid) {
  return new ExampleLP();
}

int main(int argc, char** argv) {
  tw_init(argc, argv);


  g_tw_lookahead = 100;
  g_tw_ts_end = 128000;
  g_tw_max_events_buffered = 8192;
  g_total_lps = 16;
  g_lps_per_chare = 1;


  g_initial_events = 4;
  g_mean_delay = 1000;
  g_adjusted_mean = g_mean_delay - g_tw_lookahead;

  g_type_map = example_type_map;

  tw_define_lps(sizeof(ExampleMessage), 0);
  tw_run();
  tw_end();

  return 0;
}
