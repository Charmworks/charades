#include "lp.h"
#include "pe.h"

// TODO: These should either be declared as readonlys, or moved to a group
// data structure of global constants.
extern CProxy_PE pes;
extern CProxy_LP lps;

// Temporary definition of tw_event_send. Should be moved to a different
// file...probably something like ross.C or something like that.
typedef Event tw_event;
void tw_event_send(tw_event* e) {
  int idx = e->type->map(e->dest_id);
  lps(idx).recv_event(e);
}

LP::LP() : token(this) {
  // TODO:Create the user entites associated with this chare
  // To do this we'll need to know what types to map them to, and how many
  // to create. Need to see how this is done in ROSS
}

// Entry method for sending events to LPs.
// 1) Check if the event is earlier than our earliest and update the PE.
// 2) Check to see if we need a rollback.
// 3) Push event into the priority queue.
void LP::recv_event(Event* e) {
  if (e->ts < events.top()->ts) {
    pes.ckLocalBranch()->update_next(&token, e->ts);
  }
  if (e->ts < processedEvents.back()->ts) {
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
    ////////////////////////////////////////////////////
    // TODO: The execution logic needs work
    LPData *lp = lpData[e->type->local_map(e->dest_id)];
    e->type->execute(lp, e);
    ////////////////////////////////////////////////////
    processedEvents.push_front(e);
  }
  pes.ckLocalBranch()->update_next(&token, events.top()->ts);
}

// Fossil collect all events older than the passed in GVT.
// 1) If the next event is older than the current gvt pop it and delete it.
// 2) Update the PE with our oldest unprocessed event time.
void LP::fossil_me(tw_stime gvt) {
  while (processedEvents.back()->ts <= gvt) {
    Event* e = processedEvents.back();
    processedEvents.pop_back();
    delete e;
  }
  pes.ckLocalBranch()->update_oldest(&token, processedEvents.back()->ts);
}

// Rollback all processed events up to the passed in timestamp.
// 1) If the most recent processed event is older than the timestamp, pop it.
// 2) Execute the reverse handler on the target lp and popped event.
// 3) Push the popped event onto the event priority queue.
// Note: This method is not responsible for updating the PE.
void LP::rollback_me(tw_stime ts) {
  while (processedEvents.back()->ts > ts) {
    Event* e = processedEvents.front();
    processedEvents.pop_front();
    ////////////////////////////////////////////////////
    // TODO: The execution logic needs work
    LPData *lp = lpData[e->type->local_map(e->dest_id)];
    e->type->reverse(lp, e);
    ////////////////////////////////////////////////////
    events.push(e);
  }
}
