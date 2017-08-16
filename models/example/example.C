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
    // tw_event_new now takes a template parameter for message type. It creates
    // an event with that message type, and calls the default constructor of
    // that message type to initialize the events memory.
    Event* e = tw_event_new<ExampleMessage>(gid, delay, this);
    // get_data returns a pointer to the message data, already cast to the
    // correct type. In the future, this will also include optional error
    // checking to make sure the cast is legal.
    ExampleMessage* msg = e->get_data<ExampleMessage>();
    msg->payload = counter++;
    tw_event_send(e);
  }
}
void ExampleLP::finalize() {}

void ExampleLP::forward(ExampleMessage* msg, tw_bf* bf) {
  Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
  Event* e = tw_event_new<ExampleMessage2>(neighbor, delay, this);
  ExampleMessage2* new_msg = e->get_data<ExampleMessage2>();
  new_msg->payload = msg->payload + counter++;
  tw_event_send(e);
}

void ExampleLP::reverse(ExampleMessage* msg, tw_bf* bf) {
  tw_rand_reverse_unif(rng);
  counter--;
}

void ExampleLP::commit(ExampleMessage* msg, tw_bf* bf) {}

void ExampleLP::forward(ExampleMessage2* msg, tw_bf* bf) {
  Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
  Event* e = tw_event_new<ExampleMessage>(neighbor, delay, this);
  ExampleMessage* new_msg = e->get_data<ExampleMessage>();
  new_msg->payload = msg->payload + counter++;
  tw_event_send(e);
}
void ExampleLP::reverse(ExampleMessage2* msg, tw_bf* bf) {
  tw_rand_reverse_unif(rng);
  counter--;
}

void ExampleLP::commit(ExampleMessage2* msg, tw_bf* bf) {}

void ExampleLP2::initialize() {
  counter = 0;
  neighbor = (gid + 1) % g_total_lps;

  for (int i = 0; i < g_initial_events; i++) {
    Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
    Event* e = tw_event_new<ExampleMessage2>(gid, delay, this);
    ExampleMessage2* msg = e->get_data<ExampleMessage2>();
    msg->payload = counter++;
    tw_event_send(e);
  }
}
void ExampleLP2::finalize() {}

void ExampleLP2::forward(ExampleMessage* msg, tw_bf* bf) {
  Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
  Event* e = tw_event_new<ExampleMessage2>(neighbor, delay, this);
  ExampleMessage2* new_msg = e->get_data<ExampleMessage2>();
  new_msg->payload = msg->payload + counter++;
  tw_event_send(e);
}

void ExampleLP2::reverse(ExampleMessage* msg, tw_bf* bf) {
  tw_rand_reverse_unif(rng);
  counter--;
}

void ExampleLP2::commit(ExampleMessage* msg, tw_bf* bf) {}

void ExampleLP2::forward(ExampleMessage2* msg, tw_bf* bf) {
  Time delay = g_tw_lookahead + g_adjusted_mean*tw_rand_exponential(rng, 1.0);
  Event* e = tw_event_new<ExampleMessage>(neighbor, delay, this);
  ExampleMessage* new_msg = e->get_data<ExampleMessage>();
  new_msg->payload = msg->payload + counter++;
  tw_event_send(e);
}

void ExampleLP2::reverse(ExampleMessage2* msg, tw_bf* bf) {
  tw_rand_reverse_unif(rng);
  counter--;
}

void ExampleLP2::commit(ExampleMessage2* msg, tw_bf* bf) {}

/** LP Factory */
LPBase* example_type_map(uint64_t gid) {
  if (gid % 2 == 0) {
    return new ExampleLP();
  } else {
    return new ExampleLP2();
  }
}

int main(int argc, char** argv) {
  tw_init(argc, argv);

  // All message types must be registered at the beginning to set up the
  // message dispatch framework.
  register_msg_type<ExampleMessage>();
  register_msg_type<ExampleMessage2>();

  g_tw_lookahead = 100;
  g_tw_ts_end = 128000;
  g_tw_expected_events = 8100;
  g_total_lps = 16;
  g_lps_per_chare = 1;


  g_initial_events = 4;
  g_mean_delay = 1000;
  g_adjusted_mean = g_mean_delay - g_tw_lookahead;

  g_type_map = example_type_map;

  tw_create_lps();
  tw_run();
  tw_end();

  return 0;
}
