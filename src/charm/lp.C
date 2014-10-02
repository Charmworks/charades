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
    // TODO: Why do we use the isLPSet flag rather than just setting it here?
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
LP::LP() : next_token(this), oldest_token(this), uniqID(0), cancel_q(NULL),
           min_cancel_q(DBL_MAX), enqueued_cancel_q(false), current_time(0),
           all_events(0) {
  if(isLpSet == 0) {
    lps = thisProxy;
    isLpSet = 1;
  }

  isOptimistic = PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC;
  pe = pes.ckLocalBranch();
  lp_structs.resize(PE_VALUE(g_lps_per_chare));

  // Register with the local PE so it can schedule this LP for execution, fossil
  // collection, and cancelation.
  pe->register_lp(&next_token, 0.0, &oldest_token, 0.0);

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

  // Once all LP Chares have been created and set up, return control to the
  // ROSS initialization.
  contribute(CkCallback(CkIndex_LP::stopScheduler(), thisProxy(0)));
}

void LP::stopScheduler() {
  if(tw_ismaster()) DEBUG("[%d] Stop scheduler \n", CkMyPe());
  CkExit();
}

// Call init on all LPs then stop the charm scheduler.
void LP::init() {
  curr_event = PE_VALUE(abort_event);
  if(tw_ismaster()) DEBUG("[%d] Init lps \n", CkMyPe());
  for (int i = 0 ; i < PE_VALUE(g_lps_per_chare); i++) {
    lp_structs[i].type->init(lp_structs[i].state, &lp_structs[i]);
  }
  contribute(CkCallback(CkIndex_LP::stopScheduler(), thisProxy(0)));
}

// Delete an event in our pending queue
void LP::delete_pending(Event *e) {
  if(events.top() == e) {
    events.erase(e);
    if(events.top() != NULL) {
      pe->update_next(&next_token, events.top()->ts);
    } else {
      pe->update_next(&next_token, DBL_MAX);
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
  // Copy over the relevant fields from the remote event to the local event.
  Event *e = allocateEvent(0);
  e->event_id = event->event_id;
  e->ts = event->ts;
  e->dest_lp = (tw_lpid)&lp_structs[PE_VALUE(g_local_map)(event->dest_lp)];
  e->send_pe = event->send_pe;

  if(event->isAnti) {
    // Find the corresponding real event in the avl tree, cancel it, and
    // deallocate all involved events.
    // TODO: Maybe we can get rid of the need to allocate e in the first place.
    Event *real_e = avlDelete(&all_events, e);
    tw_event_free(this, e);
    e = real_e;
    e->state.remote = 0;
    event_cancel(e);
    delete event;
  } else {
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
    if (events.top() != NULL && e->ts < events.top()->ts) {
      pe->update_next(&next_token, events.top()->ts);
    }

    // Push the event into the queue.
    events.push(e);
    e->state.owner = TW_chare_q;
  }
}

// Execute events up to timestamp ts.
// 1) Check for lazy rollbacks if optimistic
// 2) While next event is still earlier than ts:
//  2a) Pop event
//  2b) Execute event
//  2c) Free event, or put into processed queue if optimistic
// 3) Update the PE with our new earliest timestamp.
void LP::execute_me(tw_stime ts) {
  // Do a lazy check for rollbacks from short-circuit sends
  if (isOptimistic) {
    if (events.top() && processed_events.front() &&
        events.top()->ts < processed_events.front()->ts) {
      rollback_me(events.top()->ts);
    }
  }

  // TODO: Right now it seems to crash if the DBL_MAX check isn't there. This
  // will cause problems when we try to batch execute.
  while (events.top() != NULL && events.top()->ts <= ts && ts != DBL_MAX) {
    // Pull off the top event for execution
    Event* e = events.top();
    events.pop();
    current_time = e->ts;
    current_event = e;
    LPStruct* lp = (LPStruct*)e->dest_lp;
    if (isOptimistic) {
      reset_bitfields(e);
    }
    lp->type->execute(lp->state, &e->cv, tw_event_data(e), lp);

    // TODO: This may be changed when the final stats framework is done
    (PE_VALUE(netEvents))++;

    // Enqueue or deallocate the event depending on sync mode
    if (isOptimistic) {
      if (processed_events.front() == NULL) {
        pe->update_oldest(&oldest_token, e->ts);
      }
      processed_events.push_front(e);
      e->state.owner = TW_rollback_q;
    } else {
      tw_event_free(this, e);
    }
  }
  if(events.top() != NULL) {
    pe->update_next(&next_token, events.top()->ts);
  } else {
    pe->update_next(&next_token, DBL_MAX);
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
    pe->update_oldest(&oldest_token, processed_events.back()->ts);
  } else {
    pe->update_oldest(&oldest_token, gvt);
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
    // TODO: Do we also need to update current_event?
    if(processed_events.front() != NULL) {
      current_time = processed_events.front()->ts;
    }
  }
  if(processed_events.front() == NULL) {
    pe->update_oldest(&oldest_token, DBL_MAX);
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
    // TODO: Do we also need to update current_event?
    current_time = processed_events.front()->ts;
    e = processed_events.front();
    processed_events.pop_front();
  }
  assert(e == event);
  // TODO: Make sure this is only ever called in cases where we don't have to
  // push event back onto the pending queue.
  tw_event_rollback(event);
  if(processed_events.front() == NULL) {
    pe->update_oldest(&oldest_token, DBL_MAX);
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
          // TODO: Move this into rollback_me and only call if needed
          if(events.top() != NULL) {
            pe->update_next(&next_token, events.top()->ts);
          } else {
            pe->update_next(&next_token, DBL_MAX);
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
