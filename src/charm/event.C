#include "event.h"
#include "lp.h"

#include "typedefs.h"
#include "globals.h"
#include "lp_struct.h"
#include "avl_tree.h"
#include "event_buffer.h"
#include "charm_functions.h"

#include "ross_util.h"

#include <assert.h>
#include <stack>

extern CProxy_LP lps;
extern CProxy_PE pes;

void RemoteEvent::pup(PUP::er& p) {
  p | event_id;
  p | ts;
  p | send_pe;
  p | dest_lp;
  p((char*)userData, PE_VALUE(g_tw_msg_sz));
}

void operator|(PUP::er& p, Event* e) {
  // Basic pupping
  p | e->seq_num;  
  p | e->ts;

  // Pup the flat structs in Event as just plain bytes
  p((char*)&(e->state), sizeof(tw_event_state));
  p((char*)&(e->cv), sizeof(tw_bf));

  // TODO: Things that can be pointers or ints need to be handled correctly
  // TODO: These may all have to be converted to ints before pupping
  p | e->dest_lp;
  p | e->src_lp;
  p | e->send_pe;

  // Pupping the remote message data
  p | e->hasMsg;
  if (e->hasMsg) {
    if (p.isUnpacking()) {
      e->eventMsg = PE_VALUE(event_buffer)->get_remote_event();
      e->userData = e->eventMsg->userData;
    }
    p | *(e->eventMsg);
  }

  // TODO: Figure out how to pup out_msgs
  //p | out_msgs;

  // Pupping causality links
  Event* tmp;
  // If we are packing, we need to find out how many causal events there are.
  if (p.isPacking()) {
    e->pending_count = 0;
    e->processed_count = 0;
    tmp = e->caused_by_me;
    while (tmp) {
      if (tmp->state.owner = TW_chare_q) {
        e->pending_count++;
      } else if (tmp->state.owner = TW_rollback_q) {
        e->processed_count++;
      } else {
        // TODO: What to do with sent events?
      }
      tmp = tmp->cause_next;
    }
  }

  p | e->pending_count;
  p | e->processed_count;
  e->pending_indices = new unsigned[e->pending_count];
  e->processed_indices = new unsigned[e->processed_count];

  // If we are packing, fill the temporary arrays before pupping them.
  if (p.isPacking()) {
    // NOTE: This only works because causal events will either be in the
    // pending queue, or in the processed queue, but pupped before this one.
    // This also implies that the pending queue must be pupped before the
    // processed queue. If an event gets pupped before an event it caused,
    // this whole process will fall apart.
    unsigned pending_idx = 0;
    unsigned processed_idx = 0;
    tmp = e->caused_by_me;
    while (tmp) {
      if (tmp->state.owner = TW_chare_q) {
        e->pending_indices[pending_idx++] = tmp->seq_num;
      } else if (tmp->state.owner = TW_rollback_q) {
        e->processed_indices[processed_idx++] = tmp->seq_num;
      }
      tmp = tmp->cause_next;
    }
  }
  PUParray(p, e->pending_indices, e->pending_count);
  PUParray(p, e->processed_indices, e->processed_count);
  // TODO: What to do with sent events
}

Event* charm_allocate_event(int needMsg = 1) {
  Event* e;
  e = PE_VALUE(event_buffer)->get_event();
  if (needMsg) {
    e->eventMsg = PE_VALUE(event_buffer)->get_remote_event();
    e->userData = e->eventMsg->userData;
  }
  return e;
}

void charm_free_event(Event* e) {
  // If we are optimistic, the event may need to be removed from the avl tree,
  // as well as freeing any events caused by it.
  if (PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
    if(e->state.remote == 1) {
      avlDelete(&((LPStruct*)e->dest_lp)->owner->all_events, e);
    }

    Event* event = e->caused_by_me;
    while (event) {
      Event* n = event->cause_next;
      // If the owner is not TW_sent, then the event was sent locally and the
      // pointer is still being used by the receiver on this PE.
      if(event->state.owner == TW_sent) {
        charm_free_event(event);
      }
      event = n;
    }
  }

  e->state.remote = 0;
  e->state.cancel_q = 0;
  e->state.owner = TW_event_null;
  e->caused_by_me = NULL;
  e->cause_next = NULL;
  e->cancel_next = NULL;
  PE_VALUE(event_buffer)->free_event(e);
}

// Fill an event's remote message and send it.
// Returns 1 if the send was remote, 0 if it was local.
int charm_event_send(unsigned dest_peid, Event * e) {
  LP* send_pe = (LP*)(e->send_pe);
  LP* dest_pe;

  // When e is passed in, src_lp is a pointer to an lp and dest_lp is a gid.
  tw_lpid src_lp = ((LPStruct*)e->src_lp)->gid;
  tw_lpid dest_lp = e->dest_lp;

  // Attempt to get a direct pointer to the destination chare.
  // NOTE: For now we don't short circuit messages to other chares on the same
  // core to make migration easier.
  if (dest_peid == send_pe->thisIndex) {
    dest_pe = send_pe;
  } else {
    dest_pe = NULL;
  }

  // If we got a pointer, do a local send. Otherwise, send remotely.
  if (dest_pe != NULL) {
    // Check if an LP is sending to an LP other than itself (for stats).
    if (dest_lp != src_lp) {
      PE_STATS(s_nsend_loc_remote)++;
    }
    dest_pe->recv_local_event(e);
    return 0;
  } else {
    PE_STATS(s_nsend_net_remote)++;
    // Fill the fields of the charm message to prepare it for sending.
    e->eventMsg->event_id = e->event_id = send_pe->uniqID++;
    e->eventMsg->ts = e->ts;
    e->eventMsg->dest_lp = e->dest_lp;
    e->eventMsg->send_pe = e->send_pe = send_pe->thisIndex;

    lps(dest_peid).recv_remote_event(e->eventMsg);
    e->state.owner = TW_sent;
    e->eventMsg = NULL;
    return 1;
  }
}

// Allocate a new remote message, fill it based on e, and send it.
// An anti send will never be to a local chare, because locally sent events
// will never have the owner set to TW_sent.
void charm_anti_send(unsigned dest_peid, Event * e) {
  RemoteEvent * eventMsg = PE_VALUE(event_buffer)->get_remote_event();
  eventMsg->event_id = e->event_id;
  eventMsg->ts = e->ts;
  eventMsg->dest_lp = e->dest_lp;
  eventMsg->send_pe = e->send_pe;

  lps(dest_peid).recv_anti_event(eventMsg);
}

// Cancels an event by either sending an anti-message, or calling cancel_event
// on the local LP chare.
void charm_event_cancel(Event * e) {
  // Already in cancel q, return
  if(e->state.cancel_q) {
    return;
  }

  // If already sent, have charm send an anti message, and free the local event
  if(e->state.owner == TW_sent) {
    unsigned dest_peid = PE_VALUE(g_chare_map)(e->dest_lp);
    charm_anti_send(dest_peid, e);

    LP *send_pe = (LP*)((tw_lp*)e->src_lp)->owner;
    tw_event_free(send_pe, e);
    return;
  }

  // If is local, then the LP can cancel it
  LP *dest_pe = ((tw_lp*)e->dest_lp)->owner;
  dest_pe->cancel_event(e);
}

#include "event.def.h"
