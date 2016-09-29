#include "event.h"
#include "lp.h"
#include "pe.h"

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

Event* charm_allocate_event(int needMsg) {
  Event* e;
  e = PE_VALUE(event_buffer)->get_event();
  if (e == PE_VALUE(abort_event)) {
    tw_error(TW_LOC, "We are all out of events to allocate!\n");
  }
  if (needMsg) {
    e->eventMsg = PE_VALUE(event_buffer)->get_remote_event();
    e->userData = e->eventMsg->userData;
  }
  return e;
}

void charm_free_event(Event* e) {
  // If we are optimistic, the event may need to be removed from the avl tree,
  // as well as freeing any events caused by it.
  if (g_tw_synchronization_protocol == OPTIMISTIC) {
    if(e->state.avl_tree == 1) {
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

  PE_VALUE(event_buffer)->free_event(e);
}

// Fill an event's remote message and send it.
// Returns 1 if the send was remote, 0 if it was local.
int charm_event_send(unsigned dest_peid, Event * e) {
  static PE* pe = pes.ckLocalBranch();
  LP* send_pe = (LP*)(e->send_pe);
  LP* dest_pe;

  // When e is passed in, src_lp and send_pe are pointers, dest_lp is a gid.
  e->src_lp = ((LPStruct*)e->src_lp)->gid;
  e->send_pe = send_pe->thisIndex;

  // Check to see if this is a local send or not.
  if (dest_peid == send_pe->thisIndex) {
    dest_pe = send_pe;
  } else {
    dest_pe = NULL;
  }

  // If we got a pointer, do a local send. Otherwise, send remotely.
  if (dest_pe != NULL) {
    // Check if an LP is sending to an LP other than itself (for stats).
    if (e->dest_lp != e->src_lp) {
      PE_STATS(local_sends)++;
    }
    dest_pe->recv_local_event(e);
    return 0;
  } else {
    PE_STATS(remote_sends)++;
    // Fill the fields of the charm message to prepare it for sending.
    e->eventMsg->event_id = e->event_id = send_pe->uniqID++;
    e->eventMsg->ts = e->ts;
    e->eventMsg->dest_lp = e->dest_lp;
    e->eventMsg->send_pe = e->send_pe;

    pe->produce(e->eventMsg);
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
  static PE* pe = pes.ckLocalBranch();
  RemoteEvent * eventMsg = PE_VALUE(event_buffer)->get_remote_event();
  eventMsg->event_id = e->event_id;
  eventMsg->ts = e->ts;
  eventMsg->dest_lp = e->dest_lp;
  eventMsg->send_pe = e->send_pe;

  pe->produce(eventMsg);
  PE_STATS(anti_sends)++;
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
    unsigned dest_peid = g_chare_map(e->dest_lp);
    charm_anti_send(dest_peid, e);

    tw_event_free(e,false);
    return;
  }

  // If is local, then the LP can cancel it
  LP *dest_pe = ((tw_lp*)e->dest_lp)->owner;
  dest_pe->cancel_event(e);
}

#include "event.def.h"
