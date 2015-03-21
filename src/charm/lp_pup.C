#include "lp.h"
#include "pe.h"
#include "avl_tree.h"
#include "globals.h"

extern CProxy_PE pes;

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
    lp.type = PE_VALUE(g_type_map)(lp.gid);
    lp.state = malloc(lp.type->state_size);
    lp.rng = (tw_rng_stream*)malloc(sizeof(tw_rng_stream));
  }
  p((char*)lp.state, lp.type->state_size);
  p | lp.rng;
}

// Make sure we know our local pe, and construct the tokens.
LP::LP(CkMigrateMessage* m) : next_token(this), oldest_token(this),
                              cancel_q(NULL), min_cancel_q(DBL_MAX),
                              in_pe_queue(false), all_events(0),
                              current_time(0.0), current_event(NULL) {
  pe = pes.ckLocalBranch();
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
  p | current_time;

  // LP Struct Pupping (reassign owner upon unpacking)
  p | lp_structs;
  if (p.isUnpacking()) {
    for (int i = 0; i < lp_structs.size(); i++) {
      lp_structs[i].owner = this;
    }
  }

  // Event Queue Pupping
  p | events;
  if (p.isUnpacking()) {
    Event** temp_pending = events.get_temp_event_buffer();
    for (int i = 0; i < events.size(); i++) {
      reconstruct_pending_event(temp_pending[i]);
    }
    events.delete_temp_event_buffer();
    pe->update_next(&next_token, events.min());
  }
}

// Pending events need to have pointers to LPs reset, and may need to be added
// to the cancel_q and/or avl_tree.
void LP::reconstruct_pending_event(Event* e) {
  // Reconstruct pointers
  e->dest_lp = (tw_lpid)(&lp_structs[PE_VALUE(g_local_map)(e->dest_lp)]);
  if (!e->state.remote) {
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

// Relink causality for event pointers to the pending and processed queues.
void LP::reconstruct_causality(Event* e, Event** pending, Event** processed) {
/*  for (int i = 0; i < e->processed_count; i++) {
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
  delete[] e->pending_indices;*/
}

// WHEN DOING OPTIMISTIC:
// WE ALSO NEED TO ADDRESS THE CANCEL_Q, AVL_TREE, and CURRENT_EVENT
  /*if (isOptimistic) {
    p | processed_events;
    if (p.isUnpacking()) {
      // Reconstruct processed events
      Event** temp_processed = processed_events.get_temp_event_buffer();
      for (int i = 0; i < processed_events.size(); i++) {
        Event* e = temp_processed[i];
        reconstruct_event(e, temp_pending, temp_processed);
      }

      // Delete the temporary buffers.
      processed_events.delete_temp_event_buffer();

      // Update the current state of this LP both locally, and on the PE.
      if (isOptimistic) {
        pe->update_oldest(&oldest_token, processed_events.min());
        current_time = processed_events.max();
        current_event = processed_events.front();
      }
    }
  }*/

// Reset pointer fields in events, and re-add events into the avl tree/cancel_q
// if necessary.
void LP::reconstruct_processed_event(Event* e, Event** pending, Event** processed) {
  /*// Based on the owner, reset dest_lp fields to pointers.
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
  }*/
}
