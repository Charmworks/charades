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

void initialize(ExampleState* state, tw_lp* lp) {
  /** Set our LPs counter to 0, and compute the neighboring global ID */
  state->counter = 0;
  state->neighbor = (lp->gid + 1) % g_total_lps;

  /**
   * Send out g_initial_events events to start the simulation, all to our
   * neighoring LP, with payload equal to our counter, which is incremented with
   * each event sent.
   */
  for (int i = 0; i < g_initial_events; i++) {
    /**
     * Use tw_rand_exponential to generate a random double. To get deterministic
     * simulations, tw_rand_reverse_unif() needs to be called in the reverse
     * handler to reset the RNG to its earlier state.
     */
    Time delay = g_tw_lookahead + tw_rand_exponential(lp->rng, g_adjusted_mean);
    /**
     * Create a new event for our neighbor LP, at our current virtual time plus
     * delay, and with lp as the event source.
     */
    Event* e = tw_event_new(state->neighbor, delay, lp);
    /**
     * Set the message specific data, which for this model involves incrementing
     * the counter in state. This means in the reverse handler, the counter will
     * need to be decremented.
     */
    ExampleMessage* msg = (ExampleMessage*)tw_event_data(e);
    msg->payload = state->counter++;
    tw_event_send(e);
  }
}

void forward(ExampleState* state, tw_bf* bf, ExampleMessage* msg, tw_lp* lp) {
  /**
   * To forward execute an event, we just create a new event and set its
   * payload to be the sum of our current counter and the payload of the
   * incoming event. All other API calls are the same as in initialize().
   */
  Time delay = g_tw_lookahead + tw_rand_exponential(lp->rng, g_adjusted_mean);
  Event* e = tw_event_new(state->neighbor, delay, lp);
  ExampleMessage* new_msg = (ExampleMessage*)tw_event_data(e);
  new_msg->payload = msg->payload + state->counter++;
  tw_event_send(e);
}

void reverse(ExampleState* state, tw_bf* bf, ExampleMessage* msg, tw_lp* lp) {
  /**
   * To reverse an event we need to undo all of its changes on the state of the
   * simulation, including the state of the random number generators. In this
   * case it is just reversing a single RNG call, and decrementing the counter.
   */
  tw_rand_reverse_unif(lp->rng);
  state->counter--;
}

void commit(ExampleState* state, tw_bf* bf, ExampleMessage* msg, tw_lp* lp) {
  /**
   * The events in this model don't have any side-effects that can't be
   * reversed, so there is no need to do anything at commit time.
   */
}

void finalize(ExampleState* state, tw_lp* lp) {
  /** The LPs in this model don't need to do anything at finalization time */
}

/**
 * We can define as many LPTypes as we like, which consist of a set of handlers
 * and a state size. During initialization of the simulation, a type mapping
 * will be used to assign types to LPs based on their global IDs.
 */
LPType example_type = {
  (init_f) initialize,
  (event_f) forward,
  (revent_f) reverse,
  (commit_f) commit,
  (final_f) finalize,
  sizeof(ExampleState)
};

/**
 * A mapping of global ID to LPType. In this simple example, all LPs have the
 * same type.
 */
LPType* example_type_map(tw_lpid gid) {
  return &example_type;
}

/**
 * The main function is responsible for setting up the simulation. This entails
 * defining what an LP is, how many LPs there are, and how they map to chares.
 * This is also where the scheduler and GVT algorithms can be configured, before
 * making a call to tw_run() to actually start the simulation.
 */
int main(int argc, char** argv) {
  tw_init(argc, argv);

  g_tw_lookahead = 0.1;
  g_tw_ts_end = 1024.0;
  g_total_lps = 16;
  g_lps_per_chare = 1;

  g_initial_events = 4;
  g_mean_delay = 1.0;
  g_adjusted_mean = g_mean_delay - g_tw_lookahead;

  g_type_map = example_type_map;
  tw_define_lps(sizeof(ExampleMessage), 0);
  tw_run();
  tw_end();

  return 0;
}
