#include "event.h"
#include "lp.h"

#include "typedefs.h"
#include "lp_struct.h"

#include "ross_event.h"
#include "ross_util.h"

// PUP method for remote events. Called from pup_pending_event() and
// pup_processed_event().
void RemoteEvent::pup(PUP::er& p) {
  p | event_id;
  p | ts;
  p | send_pe;
  p | dest_lp;
  p((char*)userData, PE_VALUE(g_tw_msg_sz));
}

// TODO: Sent events may not need bitfields or seq_nums.
// TODO: Output messages
inline void basic_event_pup(PUP::er& p, Event* e) {
  p | e->event_id;
  p | e->ts;
  p | e->seq_num;

  p((char*)&(e->cv), sizeof(tw_bf));
  p((char*)&(e->state), sizeof(tw_event_state));

  p | e->dest_lp;
  p | e->src_lp;
  p | e->send_pe;
}

// PENDING EVENTS:
// dest_lp is definitely a pointer, src_lp and send_pe may be pointers
// Definitely have a RemoteEvent
// No causality information
void pup_pending_event(PUP::er& p, Event* e) {
  if (e->state.owner != TW_chare_q) {
    tw_error(TW_LOC, "Bad pup call, TW_chare_q != %d!\n", e->state.owner);
  }

  if (p.isPacking()) {
    e->dest_lp = ((tw_lp*)(e->dest_lp))->gid;
    if (!e->state.remote) {
      e->src_lp = ((tw_lp*)(e->src_lp))->gid;
      e->send_pe = ((LP*)(e->send_pe))->thisIndex;
    }
  }

  basic_event_pup(p, e);

  p | *(e->eventMsg);
  if (e->isUnpacking()) {
    e->userData = e->eventMsg->userData;
  }
}

// PROCESSED EVENTS:
// dest_lp is definitely a pointer, src_lp and send_pe may be pointers
// Definitely have a RemoteEvent
// Must PUP causality information
void pup_processed_event(PUP::er& p, Event* e) {
  if (e->state.owner != TW_rollback_q) {
    tw_error(TW_LOC, "Bad pup call, TW_rollback_q != %d!\n", e->state.owner);
  }

  if (p.isPacking()) {
    e->dest_lp = ((tw_lp*)(e->dest_lp))->gid;
    if (!e->state.remote) {
      e->src_lp = ((tw_lp*)(e->src_lp))->gid;
      e->send_pe = ((LP*)(e->send_pe))->thisIndex;
    }
  }

  basic_event_pup(p, e);

  p | *(e->eventMsg);
  if (e->isUnpacking()) {
    e->userData = e->eventMsg->userData;
  }

  // PUPPING THE CAUSALITY LIST
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
        tw_error(TW_LOC, "Bad owner in causality list: %d\n", tmp->state.owner);
      }
      tmp = tmp->cause_next;
    }
  }
  p | e->pending_count;
  p | e->processed_count;
  p | e->sent_count;

  // When packing and unpacking we need to create the temporary arrays.
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
        e->pending_indices[pending_idx++] = tmp->seq_num;
        unlink = true;
        if (pending_idx > e->pending_count) {
          tw_error(TW_LOC, "Mismatched number of pending events!\n");
        }
      } else if (tmp->state.owner == TW_rollback_q) {
        e->processed_indices[processed_idx++] = tmp->seq_num;
        unlink = true;
        if (processed_idx > e->processed_count) {
          tw_error(TW_LOC, "Mismatched number of processed event!\n");
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
  PUParray(p, e->pending_indices, e->pending_count);
  PUParray(p, e->processed_indices, e->processed_count);
  
  // Finally, pup the sent events in the causality list
  Event* tmp = e->caused_by_me;
  for (int i = 0; i < e->sent_count; i++) {
    if (p.isUnpacking()) {
      tmp = tw_event_new(0,0,0);
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

// SENT EVENTS:
// src_lp and send_pe definitely pointers, dest_lp definitely not
// No RemoteEvent
// No causality information
void pup_sent_event(PUP::er& p, Event* e) {
  if (e->state.owner != TW_sent) {
    tw_error(TW_LOC, "Bad pup call, TW_sent != %d!\n", e->state.owner);
  }

  if (p.isPacking()) {
    e->src_lp = ((tw_lp*)(e->src_lp))->gid;
    e->send_pe = ((LP*)(e->send_pe))->thisIndex;
  }

  basic_event_pup(p, e);
}
