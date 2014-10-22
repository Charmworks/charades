#include "lp.h"
#include "pe.h"
#include "event.h"

#include "globals.h"
#include "ross_api.h"
#include "charm_api.h"

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

tw_stime tw_now(tw_lp* lp) {
  return lp->owner->current_time;
}

Event* current_event(tw_lp* lp) {
  return lp->owner->current_event;
}

void set_current_event(tw_lp* lp, Event* event) {
  lp->owner->current_event = event;
  lp->owner->current_time = event->ts;
}

#undef PE_VALUE
#define PE_VALUE(x) pe->globals->x

// Create LPStructs based on mappings, and do initial registration with the PE.
LP::LP() : next_token(this), oldest_token(this), uniqID(0), cancel_q(NULL),
           min_cancel_q(DBL_MAX), enqueued_cancel_q(false), current_time(0),
           all_events(0) {
  if(isLpSet == 0) {
    lps = thisProxy;
    isLpSet = 1;
  }

  // Cache the pointer to the local PE chare
  pe = pes.ckLocalBranch();

  // Register with the local PE so it can schedule this LP for execution, fossil
  // collection, and cancelation.
  pe->register_lp(&next_token, 0.0, &oldest_token, 0.0);

  isOptimistic = PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC;

  // Create array of LPStructs based on globals
  lp_structs.resize(PE_VALUE(g_lps_per_chare));
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
  current_event = PE_VALUE(abort_event);
  if(tw_ismaster()) DEBUG("[%d] Init lps \n", CkMyPe());
  for (int i = 0 ; i < PE_VALUE(g_lps_per_chare); i++) {
    lp_structs[i].type->init(lp_structs[i].state, &lp_structs[i]);
  }
  contribute(CkCallback(CkIndex_LP::stopScheduler(), thisProxy(0)));
}

// Entry method for sending events to LPs.
// 1) Check if the event is earlier than our earliest and update the PE.
// 2) Check to see if we need a rollback.
// 3) Push event into the priority queue.
void LP::recv_event(RemoteEvent* event) {
  // Copy over the relevant fields from the remote event to the local event.
  Event *e = charm_allocate_event(0);
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
    real_e->state.remote = 0;
    charm_event_cancel(real_e);
    delete event;
  } else {
    e->state.remote = 1;
    e->userData = event->userData;
    e->eventMsg = event;

    // If this event is now the earliest, update the PE
    if (e->ts < events.min()) {
      pe->update_next(&next_token, e->ts);
    }

    // If optimistic, then we also have to hash the event and check for rollback
    if(isOptimistic) {
      avlInsert(&all_events, e);
      if (e->ts < current_time) {
        rollback_me(e->ts);
      }
    }

    // Push the event into the queue
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
int LP::execute_me(tw_stime ts, int max) {
  int num_executed = 0;
  while (events.size() && events.min() <= ts && num_executed != max) {
    // Pull off the top event for execution
    Event* e = events.pop();
    current_time = e->ts;
    current_event = e;
    LPStruct* lp = (LPStruct*)e->dest_lp;
    if (isOptimistic) {
      reset_bitfields(e);
    }
    lp->type->execute(lp->state, &e->cv, tw_event_data(e), lp);
    num_executed++;

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

  // Events were popped, so it is guaranteed we will need to update the PE.
  pe->update_next(&next_token, events.min());

  return num_executed;
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
    pe->update_oldest(&oldest_token, DBL_MAX);
  }
}

// Rollback all processed events up to the passed in timestamp.
void LP::rollback_me(tw_stime ts) {
  Event* e;
  while(processed_events.front() != NULL && processed_events.front()->ts > ts) {
    e = processed_events.front();
    processed_events.pop_front();
    tw_event_rollback(e);
    events.push(e);
    e->state.owner = TW_chare_q;
  }

  pe->update_next(&next_token, events.min());
  if(processed_events.front() == NULL) {
    pe->update_oldest(&oldest_token, DBL_MAX);
    current_event = NULL;
    current_time = PE_VALUE(lastGVT);
  } else {
    current_event = processed_events.front();
    current_time = current_event->ts;
  }
}

void LP::rollback_me(Event *event) {
  Event* e = processed_events.front();
  processed_events.pop_front();
  while (e != event) {
    tw_event_rollback(e);
    events.push(e);
    e->state.owner = TW_chare_q;
    e = processed_events.front();
    processed_events.pop_front();
  }
  // We've found the event in question so roll it back.
  // The caller will correctly handle what else to do with the event.
  assert(e == event);
  tw_event_rollback(event);

  // Update the queues, and current variables.
  pe->update_next(&next_token, events.min());
  if(processed_events.front() == NULL) {
    pe->update_oldest(&oldest_token, DBL_MAX);
    current_event = NULL;
    current_time = PE_VALUE(lastGVT);
  } else {
    current_event = processed_events.front();
    current_time = current_event->ts;
  }
}

void LP::cancel_event(Event* e) {
  switch (e->state.owner) {
    case TW_chare_q:
      // If the event hasn't been executed, just free it
      delete_pending(e);
      tw_event_free(this, e);
      return;
    case TW_rollback_q:
      // If the event has already been executed, add it to the cancel_q
      e->cancel_next = cancel_q;
      cancel_q = e;
      if (e->ts < min_cancel_q) {
        min_cancel_q = e->ts;
      }
      if (!enqueued_cancel_q) {
        pe->cancel_q.push_back(this);
        enqueued_cancel_q = true;
      }
      return;
    default:
      tw_error(TW_LOC, "Unknown owner in LP::cancel_event: %d", e->state.owner);
  }
}

// Delete an event in our pending queue
void LP::delete_pending(Event *e) {
  /*if(events.top() == e) {
    events.pop();
    if(events.top() != NULL) {
      pe->update_next(&next_token, events.top()->ts);
    } else {
      pe->update_next(&next_token, DBL_MAX);
    }
  } else {*/
  // TODO: Make sure this is optimized with new interface
  events.erase(e);
  pe->update_next(&next_token, events.min());
  //}
}

// TODO: Clean up this and the cancel_event method for consistency
void LP::process_cancel_q() {
  tw_event    *cev, *nev;

  // TODO: Why is this a loop?
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
          break;

        // TODO: Why is this case even here?
        case TW_chare_q:
          delete_pending(cev);
          tw_event_free(this, cev);
          break;

        default:
          tw_error(TW_LOC,
              "Event in cancel_q, but owner %d not recognized %d %d %d",
              cev->state.owner, cev->send_pe, cev->event_id, cev->ts);
      }
    }
  }
}
#include "lp.def.h"
