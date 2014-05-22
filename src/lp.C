#include "lp.h"
#include "pe.h"

extern CProxy_PE pes;
extern CProxy_LP lps;

// Temporary definition of tw_event_send. Should be moved eventually
typedef Event tw_event;
void tw_event_send(tw_event* e) {
  int idx = e->type->map(e->dest_id);
  lps(idx).recv_event(e);
}

LP::LP() {
  // Create the user entites associated with this chare
}

void LP::recv_event(Event* e) {
  if (e->ts < events.top()->ts) {
    pes.ckLocalBranch()->update_next(this, e->ts);
  }
  if (e->ts < processedEvents.back()->ts) {
    rollback_me(e->ts);
  }
  events.push(e);
}

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
  pes.ckLocalBranch()->update_next(this, events.top()->ts);
}

void LP::fossil_me(tw_stime gvt) {
  while (processedEvents.back()->ts <= gvt) {
    Event* e = processedEvents.back();
    processedEvents.pop_back();
    delete e;
  }
  pes.ckLocalBranch()->update_oldest(this, processedEvents.back()->ts);
}

void LP::rollback_me(tw_stime ts) {
  while (processedEvents.back()->ts < ts) {
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
