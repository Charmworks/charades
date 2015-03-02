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

// Pup function for tw_rng_stream in the LPStruct.
inline void operator|(PUP::er& p, tw_rng_stream* s) {
  PUParray(p, s->Ig, 4);
  PUParray(p, s->Lg, 4);
  PUParray(p, s->Cg, 4);
#ifdef RAND_NORMAL
  p | s->tw_normal_u1;
  p | s->tw_normal_u2;
  p | s->tw_normal_flipflop;
#endif
}

// When the LP chare is unpacking lp_structs, it will handle setting of the
// owner and type fields. The LPStruct pup just needs to handle the gid, state,
// and rng stream.
inline void operator|(PUP::er& p, LPStruct& lp) {
  p | lp.gid;
  if (p.isUnpacking()) {
    lp.type = PE_VALUE(g_type_map)(lp.gid);
    lp.state = malloc(lp.type->state_size);
    lp.rng = (tw_rng_stream*)malloc(sizeof(tw_rng_stream));
  }
  p((char*)lp.state, lp.type->state_size);
  p | lp.rng;
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

// The constructor called after migration is responsible for restoring the
// PE/LP relationship so that the migrated LP will still be scheduled for
// execution, fossil collection, etc.
LP::LP(CkMigrateMessage* m) : next_token(this), oldest_token(this),
                              cancel_q(NULL), min_cancel_q(DBL_MAX),
                              in_pe_queue(false), all_events(0) {
  pe = pes.ckLocalBranch();
}

// Relink causality for event pointers to the pending and processed queues.
void LP::reconstruct_causality(Event* e, Event** pending, Event** processed) {
  for (int i = 0; i < e->processed_count; i++) {
    Event* tmp = processed[e->processed_indices[i]];
    tmp->cause_next = e->caused_by_me;
    e->caused_by_me = tmp;
  }
  for (int i = 0; i < e->pending_count; i++) {
    Event* tmp = pending[e->pending_indices[i]];
    tmp->cause_next = e->caused_by_me;
    e->caused_by_me = tmp;
  }
}

// Reset pointer fields in events, and re-add events into the avl tree/cancel_q
// if necessary.
void LP::reconstruct_event(Event* e, Event** pending, Event** processed) {
  // Based on the owner, reset dest_lp fields to pointers.
  // Also make sure to rebuild causality if in the rollback queue.
  if (e->state.owner == TW_chare_q) {
    e->dest_lp = (tw_lpid)(&lp_structs[PE_VALUE(g_local_map)(e->dest_lp)]);
  } else if (e->state.owner == TW_rollback_q) {
    e->dest_lp = (tw_lpid)(&lp_structs[PE_VALUE(g_local_map)(e->dest_lp)]);
    reconstruct_causality(e, pending, processed);
  }

  // Also make sure to reset the src_lp and send_pe pointers when required.
  if (e->state.owner == TW_sent || !e->state.remote) {
    e->src_lp = (tw_lpid)(&lp_structs[PE_VALUE(g_local_map)(e->src_lp)]);
    e->send_pe = (tw_peid)this;
  }

  // Add the event to the avl_tree if necessary
  if (e->state.avl_tree) {
    avlInsert(&all_events, e);
  }

  // Add the event to the cancel_q if necessary.
  if (e->state.cancel_q) {
    add_to_cancel_q(e);
  }
}

void LP::pup(PUP::er& p) {
  CBase_LP::pup(p);
  // LPs must be unregistered from their current PE before they migrate, and
  // re-register with the new PE when they are being unpacked.
  if (p.isPacking()) {
    pe->unregister_lp(&next_token, &oldest_token);
  } else if (p.isUnpacking()) {
    pe->register_lp(&next_token, 0.0, &oldest_token, 0.0);
  }

  // Pup the basic fields
  p | isOptimistic;
  p | uniqID;

  // LP Struct Pupping (reassigns owner and type upon unpacking)
  p | lp_structs;
  if (p.isUnpacking()) {
    for (int i = 0; i < lp_structs.size(); i++) {
      lp_structs[i].owner = this;
    }
  }

  // Event Queue Pupping
  p | events;
  p | processed_events;

  // Reconstruction of events/trees based on the pupped events
  if (p.isUnpacking()) {
    Event** temp_pending = events.get_temp_event_buffer();
    Event** temp_processed = processed_events.get_temp_event_buffer();

    // Reconstruct pending events
    for (int i = 0; i < events.size(); i++) {
      Event* e = temp_pending[i];
      reconstruct_event(e, temp_pending, temp_processed);
    }

    // Reconstruct processed events
    for (int i = 0; i < processed_events.size(); i++) {
      Event* e = temp_processed[i];
      reconstruct_event(e, temp_pending, temp_processed);
    }

    // Delete the temporary buffers.
    events.delete_temp_event_buffer();
    processed_events.delete_temp_event_buffer();

    // Update the current state of this LP both locally, and on the PE.
    pe->update_next(&next_token, events.min());
    pe->update_oldest(&oldest_token, processed_events.min());
    current_time = processed_events.max();
    current_event = processed_events.front();
  }
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
    avlInsert(&all_events, e);
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

  Event* real_event = avlDelete(&all_events, key);
  charm_event_cancel(real_event);

  tw_event_free(this, key);
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
      if (processed_events.front() == NULL) {
        pe->update_oldest(&oldest_token, e->ts);
      }
      processed_events.push_front(e);
    } else {
      tw_event_free(this, e);
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
    tw_event_free(this,e);
  }
  if(processed_events.back() != NULL) {
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
      tw_event_free(this, e);
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
          tw_event_free(this, curr);
          break;

        case TW_rollback_q:
          rollback_me(curr);
          tw_event_free(this, curr);
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
