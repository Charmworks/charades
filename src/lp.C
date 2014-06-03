#include "lp.h"
#include "pe.h"
#include "event.h"

// TODO: These should either be declared as readonlys, or moved to a group
// data structure of global constants.
extern CProxy_PE pes;
extern CProxy_LP lps;

// Create LPStructs based on mappins, and do initial registration with the PE.
LP::LP() : next_token(this), oldest_token(this) {
  pes.ckLocalBranch()->register_lp(&next_token, 0.0, &oldest_token, 0.0);
}

// Entry method for sending events to LPs.
// 1) Check if the event is earlier than our earliest and update the PE.
// 2) Check to see if we need a rollback.
// 3) Push event into the priority queue.
void LP::recv_event(Event* e) {
  if (e->ts < events.top()->ts) {
    pes.ckLocalBranch()->update_next(&next_token, e->ts);
  }
  if (e->ts < processed_events.back()->ts) {
    rollback_me(e->ts);
  }
  events.push(e);
}

// Execute events up to timestamp ts.
// 1) If next event is still earlier than ts, pop it.
// 2) Execute the popped event on its destination LP.
// 3) Update the PE with our new earliest timestamp.
void LP::execute_me(tw_stime ts) {
  while (events.top()->ts <= ts) {
    Event* e = events.top();
    events.pop();
    current_time = e->ts;
    LPStruct *lp = lp_structs[e->local_id];
    lp->type->execute(lp, e);
    processed_events.push_front(e);
  }
  pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
}

// Fossil collect all events older than the passed in GVT.
// 1) If the next event is older than the current gvt pop it and delete it.
// 2) Update the PE with our oldest unprocessed event time.
void LP::fossil_me(tw_stime gvt) {
  while (processed_events.back()->ts <= gvt) {
    Event* e = processed_events.back();
    processed_events.pop_back();
    delete e;
  }
  pes.ckLocalBranch()->update_oldest(&oldest_token, processed_events.back()->ts);
}

// Rollback all processed events up to the passed in timestamp.
// 1) If the most recent processed event is older than the timestamp, pop it.
// 2) Execute the reverse handler on the target lp and popped event.
// 3) Push the popped event onto the event priority queue.
// Note: This method is not responsible for updating the PE.
void LP::rollback_me(tw_stime ts) {
  while (processed_events.back()->ts > ts) {
    Event* e = processed_events.front();
    processed_events.pop_front();
    LPStruct *lp = lp_structs[e->local_id];
    lp->type->reverse(lp, e);
    events.push(e);
  }
}
