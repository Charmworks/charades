#include "lp.h"
#include "pe.h"
#include "event.h"

#include "globals.h"
#include "ross_api.h"   // TODO: Why do we need this
#include "charm_api.h"  // TODO: Why do we need this

#include "ross_util.h"
#include "ross_random.h"
#include "ross_clcg4.h"

#include "avl_tree.h"

#include "mpi-interoperate.h"

#include <float.h>
#include <assert.h>

// Readonly variables for the global proxies.
extern CProxy_PE pes;
CProxy_LP lps;
int isLpSet = 0;

// TODO: Move this API to an appropriate place
void create_lps() {
  if (tw_ismaster()) {
    CProxy_LP::ckNew(PE_VALUE(g_num_chares));
  }
  StartCharmScheduler();
}

void init_lps() {
  if (tw_ismaster()) {
    lps.init();
  }
  StartCharmScheduler();
}

Time tw_now(tw_lp* lp) {
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

#undef PE_STATS
#define PE_STATS(x) pe->statistics->x

// Create LPStructs based on mappings, and do initial registration with the PE.
LP::LP() : next_token(this), oldest_token(this), uniqID(0), cancel_q(NULL),
           min_cancel_q(DBL_MAX), in_pe_queue(false), current_time(0),
           all_events(0) {
  if(isLpSet == 0) {
    lps = thisProxy;
    isLpSet = 1;
  }
  usesAtSync = true;

  // Cache the pointer to the local PE chare
  pe = pes.ckLocalBranch();

  // Register with the local PE so it can schedule this LP for execution, fossil
  // collection, and cancelation.
  pe->register_lp(&next_token, 0.0, &oldest_token, 0.0);

  isOptimistic = PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC;

  // Create array of LPStructs based on globals
  lp_structs.resize(PE_VALUE(g_numlp_map)(thisIndex));
  for (int i = 0; i < lp_structs.size(); i++) {
    lp_structs[i].owner = this;
    lp_structs[i].gid   = PE_VALUE(g_init_map)(thisIndex, i);
    lp_structs[i].type  = PE_VALUE(g_type_map)(lp_structs[i].gid);
    lp_structs[i].state = malloc(lp_structs[i].type->state_size);

    // Initialize the RNG streams for each LP
    if (PE_VALUE(g_tw_rng_default) == 1) {
      tw_rand_init_streams(&lp_structs[i], PE_VALUE(g_tw_nRNG_per_lp));
    }
  }

  // Once all LP Chares have been created and set up, return control to the
  // ROSS initialization.
  contribute(CkCallback(CkIndex_LP::stop_scheduler(), thisProxy(0)));
}

void LP::load_balance() {
  AtSync();
}

void LP::ResumeFromSync() {
  // TODO: This doens't have to be a broadcast
  contribute(CkCallback(CkReductionTarget(PE, resume_scheduler), pes));
}

// Call init on all LPs then stop the charm scheduler.
void LP::init() {
  current_event = PE_VALUE(abort_event);
  for (int i = 0 ; i < PE_VALUE(g_lps_per_chare); i++) {
    lp_structs[i].type->init(lp_structs[i].state, &lp_structs[i]);
  }
  contribute(CkCallback(CkIndex_LP::stop_scheduler(), thisProxy(0)));
}

void LP::stop_scheduler() {
  CkExit();
}

// Entry method for receiving remote events.
// 1) Allocate a new event and fill it based on the remote event.
// 2) Hash the event if optimistic.
// 3) Pass control to the local receive method.
void LP::recv_remote_event(RemoteEvent* event) {
  Event *e = charm_allocate_event(0);
  e->state.remote = 1;

  // Fill in event
  e->eventMsg = event;
  e->event_id = event->event_id;
  e->ts       = event->ts;
  e->send_pe  = event->send_pe;
  e->dest_lp  = event->dest_lp;
  e->userData = event->userData;

  // Hash event
  if (isOptimistic) {
    Event* anti_event = avlInsertOrDelete(&all_events, e);
    if (anti_event != NULL) {
      tw_event_free(anti_event);
      tw_event_free(e);
      return;
    }
  }

  recv_local_event(e);
}

// Local method for receiving an event.
// 1) Get the local lp_struct pointer.
// 2) Do any required PE updates or rollbacks.
// 3) Push event into the priority queue.
void LP::recv_local_event(Event* e) {
  e->dest_lp = (tw_lpid)&lp_structs[PE_VALUE(g_local_map)(e->dest_lp)];

  if (e->ts < events.min()) {
    pe->update_next(&next_token, e->ts);
  }
  if(isOptimistic && e->ts < current_time) {
    rollback_me(e->ts);
  }

  events.push(e);
}

// Entry method for receiving anti events.
// 1) Create a key event based on the remote event.
// 2) Use the key to find the real event and cancel it.
void LP::recv_anti_event(RemoteEvent* event) {
  Event* key = charm_allocate_event(0);
  key->event_id = event->event_id;
  key->ts = event->ts;
  key->send_pe = event->send_pe;

  Event* real_event = avlInsertOrDelete(&all_events, key);

  if (real_event != NULL) {
    charm_event_cancel(real_event);
    tw_event_free(key);
  }

  delete event;
}

// Execute the next event in the pending queue (returns false if no events).
// 1) Pop event
// 2) Execute event
// 3) Free event, or put into processed queue if optimistic
// 4) Update the PE with our new earliest timestamp
bool LP::execute_me() {
  if (events.size()) {
    // Pull off the top event for execution
    Event* e = events.pop();
    current_time = e->ts;
    current_event = e;
    LPStruct* lp = (LPStruct*)e->dest_lp;
    if (isOptimistic) {
      reset_bitfields(e);
    }
    lp->type->execute(lp->state, &e->cv, tw_event_data(e), lp);

    // Enqueue or deallocate the event depending on sync mode
    if (isOptimistic) {
      if (processed_events.size() == 0) {
        pe->update_oldest(&oldest_token, e->ts);
      }
      processed_events.push_front(e);
    } else {
      tw_event_free(e);
    }
    pe->update_next(&next_token, events.min());
    return true;
  }
  return false;
}

// Rollback all processed events up to the passed in timestamp.
void LP::rollback_me(tw_stime ts) {
  Event* e;
  PE_STATS(s_rb_total)++;
  PE_STATS(s_rb_primary)++;
  while(processed_events.size() && processed_events.front()->ts > ts) {
    e = processed_events.pop_front();
    tw_event_rollback(e);
    events.push(e);
  }

  pe->update_next(&next_token, events.min());
  if(processed_events.front() == NULL) {
    pe->update_oldest(&oldest_token, DBL_MAX);
    current_event = NULL;
    current_time = PE_VALUE(g_last_gvt);
  } else {
    current_event = processed_events.front();
    current_time = current_event->ts;
  }
}

// Rollback until we get to event, and roll it back.
void LP::rollback_me(Event *event) {
  PE_STATS(s_rb_total)++;
  PE_STATS(s_rb_secondary)++;
  Event* e = processed_events.pop_front();
  while (e != event) {
    tw_event_rollback(e);
    events.push(e);
    e = processed_events.pop_front();
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
    current_time = PE_VALUE(g_last_gvt);
  } else {
    current_event = processed_events.front();
    current_time = current_event->ts;
  }
}

// Fossil collect all events older than the passed in GVT.
// 1) If the next event is older than the current gvt pop it and delete it.
// 2) Update the PE with our oldest unprocessed event time.
void LP::fossil_me(tw_stime gvt) {
  Event* e;
  while (processed_events.back() != NULL && processed_events.back()->ts < gvt) {
    e = processed_events.pop_back();
    tw_event_free(e);
  }
  if(processed_events.size()) {
    pe->update_oldest(&oldest_token, processed_events.back()->ts);
  } else {
    pe->update_oldest(&oldest_token, DBL_MAX);
  }
}

// Cancel the event e, which should be stored on this chare.
void LP::cancel_event(Event* e) {
  switch (e->state.owner) {
    case TW_chare_q:
      // If the event hasn't been executed, just free it
      delete_pending(e);
      tw_event_free(e);
      return;
    case TW_rollback_q:
      // If the event has already been executed, add it to the cancel_q
      add_to_cancel_q(e);
      return;
    default:
      tw_error(TW_LOC, "Unknown owner in LP::cancel_event: %d", e->state.owner);
      return;
  }
}

void LP::add_to_cancel_q(Event* e) {
  e->state.cancel_q = 1;
  e->cancel_next = cancel_q;
  cancel_q = e;
  if (!in_pe_queue) {
    min_cancel_q = e->ts;
    in_pe_queue = true;
    pe->add_to_cancel_q(this);
  } else if (e->ts < min_cancel_q) {
    min_cancel_q = e->ts;
    pe->update_min_cancel(min_cancel_q);
  }
}

// Delete an event in our pending queue.
void LP::delete_pending(Event *e) {
  events.erase(e);
  pe->update_next(&next_token, events.min());
}

// Cancel all events in the cancel queue.
// NOTE: Even though only events in the rollback queue are place in the cancel
// queue, they can be rolled back during execution and end up back in the
// chare queue.
void LP::process_cancel_q() {
  Event *curr, *next;
  curr = cancel_q;

  while (cancel_q) {
    cancel_q = NULL;
    min_cancel_q = DBL_MAX;
    in_pe_queue = false;

    while(curr) {
      next = curr->cancel_next;
      switch (curr->state.owner) {
        case TW_chare_q:
          delete_pending(curr);
          tw_event_free(curr);
          break;

        case TW_rollback_q:
          rollback_me(curr);
          tw_event_free(curr);
          break;

        default:
          tw_error(TW_LOC,
              "Unknown event owner in cancel_q: %d %d %d %f",
              curr->state.owner, curr->send_pe, curr->event_id, curr->ts);
          break;
      }
      curr = next;
    }
  }
}

#include "lp.def.h"
