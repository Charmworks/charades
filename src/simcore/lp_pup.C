#include "lp.h"

#include "avl_tree.h"
#include "globals.h"
#include "scheduler.h"
#include "ross_clcg4.h"

// Pup function for tw_rng_stream in the LPStruct.
void operator|(PUP::er& p, tw_rng_stream* s) {
  PUParray(p, s->Ig, 4);
  PUParray(p, s->Lg, 4);
  PUParray(p, s->Cg, 4);
#ifdef RAND_NORMAL
  p | s->tw_normal_u1;
  p | s->tw_normal_u2;
  p | s->tw_normal_flipflop;
#endif
}

// When the LP chare is unpacking lp_structs, it will handle the owner field.
// The LPStruct pup just needs to handle the gid, state, type, and rng stream.
void operator|(PUP::er& p, LPStruct& lp) {
  p | lp.gid;
  if (p.isUnpacking()) {
    lp.type = g_type_map(lp.gid);
    lp.state = malloc(lp.type->state_size);
    lp.rng = (tw_rng_stream*)malloc(sizeof(tw_rng_stream));
  }
  p((char*)lp.state, lp.type->state_size);
  p | lp.rng;
}

// Make sure we know our local pe, and construct the tokens.
LP::LP(CkMigrateMessage* m) : next_token(this),
                              min_cancel_q(DBL_MAX),
                              all_events(0), current_time(0.0),
                              current_event(NULL) {
  scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
}

void LP::pup(PUP::er& p) {
  CBase_LP::pup(p);
  // LPs must be unregistered from their current PE before they migrate, and
  // re-register with the new PE when they are being unpacked.
  if (p.isPacking()) {
    scheduler->unregister_lp(&next_token);
  } else if (p.isUnpacking()) {
    scheduler->register_lp(&next_token, 0.0);
  }

  // Pup the basic fields
  p | isOptimistic;
  p | uniqID;
  p | current_time;

  // LP Struct Pupping (reassign owner upon unpacking)
  p | lp_structs;
  if (p.isUnpacking()) {
    for (int i = 0; i < lp_structs.size(); i++) {
      lp_structs[i].owner = this;
    }
  }

  // Event Queue Pupping
  Event** pending;
  Event** processed;

  p | events;
  if (p.isUnpacking()) {
    pending = events.get_temp_event_buffer();
    for (int i = 0; i < events.size(); i++) {
      reconstruct_pending_event(pending[i]);
    }
    scheduler->update_next(&next_token, events.min());
  }

  if (isOptimistic) {
    p | processed_events;
    if (p.isUnpacking()) {
      processed = processed_events.get_temp_event_buffer();
      for (int i = 0; i < processed_events.size(); i++) {
        reconstruct_processed_event(processed[i], pending, processed);
      }
      current_event = processed_events.front();
    }
  }

  if (p.isUnpacking()) {
    events.delete_temp_event_buffer();
    if (isOptimistic) {
      processed_events.delete_temp_event_buffer();
    }
  }
}

// Pending events need to have pointers to LPs reset, and may need to be added
// to the cancel_q and/or avl_tree.
void LP::reconstruct_pending_event(Event* e) {
  // Reconstruct pointers
  e->dest_lp = (tw_lpid)(&lp_structs[g_local_map(e->dest_lp)]);

  // Add the event to the avl_tree if necessary
  if (e->state.avl_tree) {
    avlInsert(&all_events, e);
  }

  // Add the event to the cancel_q if necessary.
  if (e->state.cancel_q) {
    add_to_cancel_q(e);
  }
}

// Reset pointer fields in events, and re-add events into the avl tree/cancel_q
// if necessary.
void LP::reconstruct_processed_event(Event* e, Event** pending, Event** processed) {
  // Reconstruct pointers
  e->dest_lp = (tw_lpid)(&lp_structs[g_local_map(e->dest_lp)]);
  reconstruct_causality(e, pending, processed);

  // Add the event to the avl_tree if necessary
  if (e->state.avl_tree) {
    avlInsert(&all_events, e);
  }

  // Add the event to the cancel_q if necessary.
  if (e->state.cancel_q) {
    add_to_cancel_q(e);
  }
}

// Relink causality for event pointers to the pending and processed queues.
void LP::reconstruct_causality(Event* e, Event** pending, Event** processed) {
  for (int i = 0; i < e->processed_count; i++) {
    Event* tmp = processed[e->processed_indices[i]];
    tmp->cause_next = e->caused_by_me;
    e->caused_by_me = tmp;
  }
  delete[] e->processed_indices;
  for (int i = 0; i < e->pending_count; i++) {
    Event* tmp = pending[e->pending_indices[i]];
    tmp->cause_next = e->caused_by_me;
    e->caused_by_me = tmp;
  }
  delete[] e->pending_indices;
}
