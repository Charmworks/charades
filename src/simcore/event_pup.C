#include "event.h"

#include "globals.h"
#include "lp.h"
#include "typedefs.h"

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
      tmp = event_alloc();
    }
    tmp->pup(p);
    if (p.isUnpacking()) {
      tmp->cause_next = e->caused_by_me;
      e->caused_by_me = tmp;
    } else {
      tmp = tmp->cause_next;
    }
  }
}
