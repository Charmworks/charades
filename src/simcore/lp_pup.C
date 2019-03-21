#include "lp.h"

#include "avl_tree.h"
#include "factory.h"
#include "globals.h"
#include "mapper.h"
#include "scheduler.h"

// Make sure we know our local pe, and construct the tokens.
LPChare::LPChare(CkMigrateMessage* m) : next_token(this),
                              cancel_q(NULL), min_cancel_q(TIME_MAX),
                              all_events(0), current_time(0.0),
                              current_event(NULL) {
  scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
}

void LPChare::pup(PUP::er& p) {
  CBase_LPChare::pup(p);
  // LPs must be unregistered from their current PE before they migrate, and
  // re-register with the new PE when they are being unpacked.
  if (p.isPacking()) {
    scheduler->unregister_lp(&next_token);
  } else if (p.isUnpacking()) {
    scheduler->register_lp(&next_token, 0.0);
  }

  // Pup the basic fields
  p | isOptimistic;
  p | current_time;
  p | next_event_id;

  // LP Struct Pupping (reassign owner upon unpacking)
  lp_structs.resize(g_lp_mapper->get_num_lps(thisIndex));
  for (int i = 0; i < lp_structs.size(); i++) {
    if (p.isUnpacking()) {
      uint64_t gid = g_lp_mapper->get_global_id(thisIndex, i);
      lp_structs[i] = g_lp_factory->create_lp(gid);
    }
    lp_structs[i]->owner = this;
    lp_structs[i]->pup(p);
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
void LPChare::reconstruct_pending_event(Event* e) {
  // Reconstruct pointers
  e->owner = lp_structs[g_lp_mapper->get_local_id(e->dest_lp)];

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
void LPChare::reconstruct_processed_event(Event* e, Event** pending, Event** processed) {
  // Reconstruct pointers
  e->owner = lp_structs[g_lp_mapper->get_local_id(e->dest_lp)];
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
void LPChare::reconstruct_causality(Event* e, Event** pending, Event** processed) {
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
