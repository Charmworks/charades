/**
 * \file example.h
 * Struct declarations used by the model. Specifically, here is where we would
 * usually define the LP state structs and event message structs. Also may
 * include global variable decls and/or method declarations.
 */

#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "ross_api.h"

/**
 * This is the type of state kept for each LP. In this simple example, each LP
 * stores the ID of its neighboring LP and a counter of events it has sent.
 */
struct ExampleState {
  int neighbor; ///< ID of the neighboring LP
  int counter;  ///< Number of events that have been sent
};

/**
 * This is the type of the model specific data sent with each event. In this
 * simple example, this is just a single number. The payload of each event is
 * calculated as the sum of the received event that caused the new event to be
 * sent, and the current counter value of the receiving LP.
 */
struct ExampleMessage {
  int payload;  ///< Payload of the event
};

/**
 * \name LP Handlers
 *
 * To define an LP, you need 5 different handlers: initialize, forward, reverse,
 * commit, and finalize. Not all of these handlers need to have non-empty bodies
 * depending on the functionality your simulation requires.
 *
 * Initialize and forward always need code, and make up the core of an LP
 * defintion:
 * - Intialize is called before the simulation begins, and is used to
 *   set up the initial LP state and send out initial messages. Without initial
 *   messages the simulation wouldn't have anything to run and would immediately
 *   end.
 * - Forward is called to handle every event sent to the passed in LP.
 *
 * Reverse and commit handlers are only used if you need to run your simulation
 * with optimistic synchronization.
 * - Reverse is required, and called to reverse the effects of the passed in
 *   event on the passed in LP. In order to do so, it is often the case that
 *   the bitfield is set in forward save information about branches and loops.
 * - Commit is optional, and is called after GVT on each event that is ready to
 *   be committed. Any non-reverisble side effects such as I/O should be done
 *   in this function. If there are no such side-effects, the function can still
 *   be left empty.
 *
 * The finalize handler is called after the simulation, and can be used for
 * things like statistics collection and logging, but can be left empty if
 * needed.
 *////@{

/**
 * Set up the initial state and send out initial events.
 * \param [out] state a pointer to the LPs state
 * \param [in]  lp a pointer to the LPStruct this handler is being called for
 */
void initialize(ExampleState* state, tw_lp* lp);

/**
 * Execute an event by updating the state and bitfield based on the message.
 * \param [in,out]  state a pointer to the LPs state
 * \param [out]     bf the bitfield correspdonding to this message
 * \param [in]      msg the message data for the event being executed
 * \param [in]      lp a pointer to the LPStruct the event was sent to
 */
void forward(ExampleState* state, tw_bf* bf, ExampleMessage* msg, tw_lp* lp);

/**
 * Reverse an event by undoing all of its effects on the state and the RNG.
 * \param [in,out]  state a pointer to the LPs state
 * \param [in]      bf the bitfield correspdonding to this message
 * \param [in]      msg the message data for the event being executed
 * \param [in]      lp a pointer to the LPStruct the event was sent to
 */
void reverse(ExampleState* state, tw_bf* bf, ExampleMessage* msg, tw_lp* lp);

/**
 * Commit any side effects of the event that can't be speculative.
 * \param [in,out]  state a pointer to the LPs state
 * \param [in]      bf the bitfield correspdonding to this message
 * \param [in]      msg the message data for the event being executed
 * \param [in]      lp a pointer to the LPStruct the event was sent to
 */
void commit(ExampleState* state, tw_bf* bf, ExampleMessage* msg, tw_lp* lp);

/**
 * Do any post processing after simulation, such as logging or stat collection.
 * \param state a pointer to the LPs state
 * \param lp a pointer to the LPStruct this handler is being called for
 */
void finalize(ExampleState* state, tw_lp* lp);
///@}

#endif
