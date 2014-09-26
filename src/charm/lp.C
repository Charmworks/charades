#include "lp.h"
#include "pe.h"
#include "event.h"

#include "globals.h"
#include "ross_api.h"

#include "ross_util.h"
#include "ross_random.h"
#include "ross_clcg4.h"
#include "avl_tree.h"

#include "float.h"
#include "assert.h"

#include "mpi-interoperate.h"

// Readonly variables for the global proxies.
extern CProxy_PE pes;
CProxy_LP lps;
int isLpSet = 0;

// This is the API which allows the ROSS code to initialize the Charm backend.
void create_lps() {
  if (tw_ismaster()) {
    CProxy_LP::ckNew(PE_VALUE(g_num_lp_chares));
  }
  StartCharmScheduler();
}

void init_lps() {
  if (tw_ismaster()) {
    lps.init();
  }
  StartCharmScheduler();
}

// Create LPStructs based on mappings, and do initial registration with the PE.
LP::LP() : next_token(this), oldest_token(this), uniqID(0), cancel_q(NULL), min_cancel_q(DBL_MAX),
           enqueued_cancel_q(false), current_time(0), all_events(0) {
  if(isLpSet == 0) {
    lps = thisProxy;
    isLpSet = 1;
  }

  lp_structs.resize(PE_VALUE(g_lps_per_chare));
  // Register with the local PE so it can schedule this LP for execution, fossil
  // collection, and cancelation.
  pes.ckLocalBranch()->register_lp(&next_token, 0.0, &oldest_token, 0.0);
  DEBUG("[%d] Registered with PE for %d lps - %d \n", CkMyPe(), PE_VALUE(g_lps_per_chare), events.size());

  // Create array of LPStructs based on globals
  for (int i = 0; i < PE_VALUE(g_lps_per_chare); i++) {
    lp_structs[i].owner = this;
    lp_structs[i].gid = PE_VALUE(g_init_map)(thisIndex, i);
    DEBUG("[%d] Created LP %d \n", CkMyPe(), lp_structs[i].gid);
    lp_structs[i].type = PE_VALUE(g_type_map)(lp_structs[i].gid);
    lp_structs[i].state = malloc(lp_structs[i].type->state_size);

    // Initialize the RNG streams for each LP
    if (PE_VALUE(g_tw_rng_default) == 1) {
      tw_rand_init_streams(&lp_structs[i], PE_VALUE(g_tw_nRNG_per_lp));
    }
  }
  if(tw_ismaster()) DEBUG("[%d] Created LPs \n", CkMyPe());
  contribute(CkCallback(CkIndex_LP::stopScheduler(), thisProxy(0)));
}

void LP::stopScheduler() {
  if(tw_ismaster()) DEBUG("[%d] Stop scheduler \n", CkMyPe());
  CkExit();
}

void LP::init() {
  currEvent = PE_VALUE(abort_event);
  if(tw_ismaster()) DEBUG("[%d] Init lps \n", CkMyPe());
  for (int i = 0 ; i < PE_VALUE(g_lps_per_chare); i++) {
    lp_structs[i].type->init(lp_structs[i].state, &lp_structs[i]);
  }
  contribute(CkCallback(CkIndex_LP::stopScheduler(), thisProxy(0)));
}

/* Delete an event in our pending queue */
void LP::delete_pending(Event *e) {
  if(events.top() == e) {
    events.erase(e);
    if(events.top() != NULL) {
      pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
    } else {
      pes.ckLocalBranch()->update_next(&next_token, DBL_MAX);
    }
  } else {
    events.erase(e);
  }
}

// Entry method for sending events to LPs.
// 1) Check if the event is earlier than our earliest and update the PE.
// 2) Check to see if we need a rollback.
// 3) Push event into the priority queue.
void LP::recv_event(RemoteEvent* event) {
  // TODO (nikhil): Difference between tw_event_new and allocate event?
  // Copy over the relevant fields from the remote event to the local event.
  DEBUG2("[%d] Received event from chare %d for %lf - %d \n", CkMyPe(), event->send_pe, event->ts, events.size());
  Event *e = allocateEvent(0);
  e->event_id = event->event_id;
  e->ts = event->ts;
  // TODO (eric): The use of map here could be cleaned up.
  e->dest_lp = (tw_lpid)&lp_structs[PE_VALUE(g_local_map)(event->dest_lp)];
  e->send_pe = event->send_pe;

  DEBUG3("[%d] Recv %d %d %d %lf\n",CkMyPe(), event->send_pe, event->event_id, event->isAnti, event->ts);

  if(event->isAnti) {
    // Find the corresponding real event in the avl tree, cancel it, and
    // deallocate all involved events.
    Event *real_e = avlDelete(&all_events, e);
    tw_event_free(this,e);
    e = real_e;
    e->state.remote = 0;
    event_cancel(e);
    DEBUG2("[%d] Went into anti %d %d %d \n", CkMyPe(), e->send_pe, e->event_id, e->state.owner);
    delete event;
  } else {
    DEBUG2("[%d,%d] Went into regular \n", CkMyPe(), thisIndex);
    e->state.remote = 1;
    if(PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
      avlInsert(&all_events, e);
    }
    e->userData = event->userData;
    e->eventMsg = event;

    // Check the timestamps in the queues to see if updates or rollbacks
    // need to be performed.
    if (processed_events.front() != NULL && e->ts < processed_events.front()->ts) {
      rollback_me(e->ts);
    }

    // Push the event into the queue and set its owner field.
    events.push(e);
    e->state.owner = TW_chare_q;
    pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
  }
}

void LP::execute_me_no_save(tw_stime ts) {
  // Pop the top event, update current time and event, then execute.
  while (events.top() != NULL && events.top()->ts <= ts && ts != DBL_MAX) {
    Event* e = events.top();
    events.pop();
    current_time = e->ts;
    currEvent = e;
    LPStruct* lp = (LPStruct*)e->dest_lp;
    lp->type->execute(lp->state, &e->cv, tw_event_data(e), lp);
    (PE_VALUE(netEvents))++;
    tw_event_free(this, e);
  }
  if(events.top() != NULL) {
    pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
  } else {
    pes.ckLocalBranch()->update_next(&next_token, DBL_MAX);
  }
}

// Execute many events, upto the given timestamp
// events are freed after being executed
// Returns the number of executed events
int LP::execute_many_no_save(Time until) {
  if (until == DBL_MAX) {
    printf("ALERT: lp executing all events until DBL_MAX!\n");
  }
  int event_counter = 0;
  while (events.top() != NULL && events.top()->ts <= until) {
    currEvent = events.pop();
    current_time = currEvent->ts;
    LPStruct* lp = (LPStruct*)currEvent->dest_lp;

    reset_bitfields(currEvent);
    lp->type->execute(lp->state, &currEvent->cv, tw_event_data(currEvent), lp);
    event_counter++;
    tw_event_free(this, currEvent);
  }
  return event_counter;
}


// Execute events up to timestamp ts.
// 1) If next event is still earlier than ts, pop it.
// 2) Execute the popped event on its destination LP.
// 3) Update the PE with our new earliest timestamp.
void LP::execute_me(tw_stime ts) {
  while (events.top() != NULL && events.top()->ts <= ts && ts != DBL_MAX) {
    Event* e = events.top();
    events.pop();
    current_time = e->ts;
    currEvent = e;
    LPStruct* lp = (LPStruct*)e->dest_lp;
    reset_bitfields(e);
    lp->type->execute(lp->state, &e->cv, tw_event_data(e), lp);
    (PE_VALUE(netEvents))++;

    // Since we're doing optimistic execution, save the event for later.
    processed_events.push_front(e);
    e->state.owner = TW_rollback_q;
  }
  if(events.top() != NULL) {
    pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
  } else {
    pes.ckLocalBranch()->update_next(&next_token, DBL_MAX);
  }
}

// Fossil collect all events older than the passed in GVT.
// 1) If the next event is older than the current gvt pop it and delete it.
// 2) Update the PE with our oldest unprocessed event time.
void LP::fossil_me(tw_stime gvt) {
  while (processed_events.back() != NULL && processed_events.back()->ts < gvt) {
    Event* e = processed_events.back();
    processed_events.pop_back();
    tw_event_free(this,e);
  }
  if(processed_events.back() != NULL) {
    pes.ckLocalBranch()->update_oldest(&oldest_token, processed_events.back()->ts);
  } else {
    pes.ckLocalBranch()->update_oldest(&oldest_token, gvt);
  }
}

// Rollback all processed events up to the passed in timestamp.
// 1) If the most recent processed event is older than the timestamp, pop it.
// 2) Execute the reverse handler on the target lp and popped event.
// 3) Push the popped event onto the event priority queue.
void LP::rollback_me(tw_stime ts) {
  Event* e;
  while(processed_events.front() != NULL && processed_events.front()->ts > ts) {
    e = processed_events.front();
    processed_events.pop_front();
    tw_event_rollback(e);
    events.push(e);
    e->state.owner = TW_chare_q;
    if(processed_events.front() == NULL) break;
    current_time = processed_events.front()->ts;
  }
  if(processed_events.front() == NULL) {
    pes.ckLocalBranch()->update_oldest(&oldest_token, PE_VALUE(lastGVT));
    current_time = PE_VALUE(lastGVT);
  }
}

void LP::rollback_me(Event *event) {
  Event* e = processed_events.front();
  processed_events.pop_front();
  while (e != event) {
    tw_event_rollback(e);
    events.push(e);
    e->state.owner = TW_chare_q;
    current_time = processed_events.front()->ts;
    e = processed_events.front();
    processed_events.pop_front();
  }
  assert(e == event);
  tw_event_rollback(event);
  if(processed_events.front() == NULL) {
    pes.ckLocalBranch()->update_oldest(&oldest_token, PE_VALUE(lastGVT));
    current_time = PE_VALUE(lastGVT);
  } else {
    current_time = processed_events.front()->ts;
  }
}

void LP::process_cancel_q() {
  tw_event    *cev, *nev;

  while (cancel_q) {
    cev = cancel_q;
    cancel_q = NULL;
    min_cancel_q = DBL_MAX;

    for (; cev; cev = nev) {
      nev = cev->cancel_next;

      switch (cev->state.owner) {
        case TW_rollback_q:
          rollback_me(cev);
          tw_event_free(this, cev);
          if(events.top() != NULL) {
            pes.ckLocalBranch()->update_next(&next_token, events.top()->ts);
          } else {
            pes.ckLocalBranch()->update_next(&next_token, DBL_MAX);
          }
          break;

        case TW_chare_q:
          delete_pending(cev);
          tw_event_free(this, cev);
          break;

        default:
          tw_error(TW_LOC, "Event in cancel_q, but owner %d not recognized %d %d %d", cev->state.owner, cev->send_pe, cev->event_id, cev->ts);
      }
    }
  }
}
#include "lp.def.h"
