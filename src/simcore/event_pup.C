#include "event.h"

#include "globals.h"
#include "lp.h"
#include "typedefs.h"

#if 0
// PUP method for remote events. Called from pup_pending_event() and
// pup_processed_event().
void RemoteEvent::pup(PUP::er& p) {
  p | ts;
  p | event_id;
  p | src_lp;
  p | dest_lp;
  p((char*)userData, g_tw_msg_sz);
}
PUPbytes(tw_event_state);
PUPbytes(tw_bf);

// Pup the basic parts needed by every event, which are just the fields used
// as the event key (event_id, ts, src_lp). Also need dest_lp (not sure why).
inline void basic_event_pup(PUP::er& p, Event* e) {
  p | e->ts;
  p | e->event_id;
  p | e->src_lp;
  p | e->dest_lp;
}

// PENDING EVENTS:
// Doesn't need bitfield, does need state.
// Doesn't need src_lp, dest_lp is a pointer.
// Guaranteed to have a RemoteEvent.
// Pointers handled by LP, PendingQueue.
// Need the index for PendingHeap, and also for processed events causality.
// No other causality info is needed.
void pup_pending_event(PUP::er& p, Event* e) {
  uint64_t dest_lp;
  if (p.isPacking()) {
    dest_lp = e->dest_lp;
    e->dest_lp = ((LPBase*)e->dest_lp)->gid;
  }
  basic_event_pup(p, e);
  if (p.isPacking()) {
    e->dest_lp = dest_lp;
  }
  p | e->state;
  p | e->index;
  p | *(e->eventMsg);
}

// PROCESSED EVENTS:
// Does need bitfield, does need state.
// Doesn't need src_lp, dest_lp is a pointer.
// Guaranteed to have a RemoteEvent.
// Pointers handled by LP and ProcessedQueue.
// Need the index for ProcessedQueue, and also for processed events causality.
// Causality pupper needs to be called to pack causality info.
void pup_processed_event(PUP::er& p, Event* e) {
  uint64_t dest_lp;
  if (p.isPacking()) {
    dest_lp = e->dest_lp;
    e->dest_lp = ((LPBase*)e->dest_lp)->gid;
  }
  basic_event_pup(p, e);
  if (p.isPacking()) {
    e->dest_lp = dest_lp;
  }
  p | e->state;
  p | e->cv;
  p | e->index;
  p | *(e->eventMsg);
  pup_causality(p, e);
}

// SENT EVENTS:
// Only needs basic info and to reset state.owner.
void pup_sent_event(PUP::er& p, Event* e) {
  basic_event_pup(p, e);
  if (p.isUnpacking()) {
    e->state.owner = TW_sent;
  }
}

// CAUSALITY LIST:
// Basically, we pack up two extra arrays that hold causality info.
// The first array holds the index field of pending events, the second holds the
// index field of processed events. The LP will rebuild the causality lists once
// all events have been unpacked by using these temporary arrays. Sent events
// are just pupped normally.
void pup_causality(PUP::er& p, Event* e) {
  // First determine how much space we'll need for causality info
  if (p.isSizing()) {
    // TODO: Why not keep track of these in link_causality
    e->pending_count = 0;
    e->processed_count = 0;
    e->sent_count = 0;
    Event* tmp = e->caused_by_me;
    while (tmp) {
      if (tmp->state.owner == TW_chare_q) {
        e->pending_count++;
      } else if (tmp->state.owner == TW_rollback_q) {
        e->processed_count++;
      } else if (tmp->state.owner == TW_sent) {
        e->sent_count++;
      } else {
        CkAbort("Bad owner in causality list\n");
      }
      tmp = tmp->cause_next;
    }
  }

  // Pup the counts of events
  p | e->pending_count;
  p | e->processed_count;
  p | e->sent_count;

  // When packing and unpacking we need to create the temporary arrays.
  // These are freed after packing is complete, and after causality is
  // reconstructed during unpacking.
  if (!p.isSizing()) {
    e->pending_indices = new unsigned[e->pending_count];
    e->processed_indices = new unsigned[e->processed_count];
  }

  // Fill the temporary arrays, and unlink all non-sent events from the
  // causality list.
  if (p.isPacking()) {
    unsigned pending_idx = 0;
    unsigned processed_idx = 0;
    Event* tmp = e->caused_by_me;
    Event* prev = NULL;
    while (tmp) {
      bool unlink = false;
      if (tmp->state.owner == TW_chare_q) {
        e->pending_indices[pending_idx++] = tmp->index;
        unlink = true;
        if (pending_idx > e->pending_count) {
          CkAbort("Mismatched number of pending events!\n");
        }
      } else if (tmp->state.owner == TW_rollback_q) {
        e->processed_indices[processed_idx++] = tmp->index;
        unlink = true;
        if (processed_idx > e->processed_count) {
          CkAbort("Mismatched number of processed event!\n");
        }
      }
      if (unlink) {
        if (prev == NULL) {
          e->caused_by_me = tmp->cause_next;
        } else {
          prev->cause_next = tmp->cause_next;
        }
      } else {
        prev = tmp;
      }
      tmp = tmp->cause_next;
    }
  }

  // PUP the arrays and delete if we are done with them.
  PUParray(p, e->pending_indices, e->pending_count);
  PUParray(p, e->processed_indices, e->processed_count);
  if (p.isPacking()) {
    delete[] e->pending_indices;
    delete[] e->processed_indices;
  }

  // Finally, pup the sent events in the causality list
  Event* tmp = e->caused_by_me;
  for (int i = 0; i < e->sent_count; i++) {
    if (p.isUnpacking()) {
      tmp = charm_allocate_event(0);
    }
    pup_sent_event(p, tmp);
    if (p.isUnpacking()) {
      tmp->cause_next = e->caused_by_me;
      e->caused_by_me = tmp;
    } else {
      tmp = tmp->cause_next;
    }
  }
}
#endif
