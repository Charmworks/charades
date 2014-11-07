#include "NDMeshStreamer.h"
#include "event.h"
#include "pe.h"
#include "lp.h"

#include "typedefs.h"
#include "globals.h"
#include "lp_struct.h"
#include "avl_tree.h"
#include "event_buffer.h"

#include "ross_util.h"

#include <assert.h>
#include <stack>

extern CProxy_LP lps;
extern CProxy_PE pes;
CProxy_ArrayMeshStreamer<RemoteEvent, int, LP, 
                  SimpleMeshRouter> aggregator;

Event* charm_allocate_event(int needMsg = 1) {
  Event* e;
  e = PE_VALUE(event_buffer)->get_event();
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
  RemoteEvent eventMsg;
  LP* send_pe = (LP*)(e->send_pe);
  LP* dest_pe;

  // Attempt to get a direct pointer to the destination chare.
  if (dest_peid == send_pe->thisIndex) {
    dest_pe = send_pe;
  } else {
    dest_pe = lps(dest_peid).ckLocal();
  }

  // If we got a pointer, do a local send. Otherwise, send remotely.
  if (dest_pe != NULL) {
    dest_pe->recv_local_event(e);
    return 0;
  } else {
    // Fill the fields of the charm message to prepare it for sending.
    eventMsg.event_id = e->event_id = send_pe->uniqID++;
    eventMsg.ts = e->ts;
    eventMsg.dest_lp = e->dest_lp;
    eventMsg.send_pe = e->send_pe = send_pe->thisIndex;
    eventMsg.isAnti = 0;

    get_aggregator()->insertData(eventMsg, dest_peid);
    e->state.owner = TW_sent;
    return 1;
  }
}

// Allocate a new remote message, fill it based on e, and send it.
// An anti send will never be to a local chare, because locally sent events
// will never have the owner set to TW_sent.
void charm_anti_send(unsigned dest_peid, Event * e) {
  RemoteEvent eventMsg;
  eventMsg.event_id = e->event_id;
  eventMsg.ts = e->ts;
  eventMsg.dest_lp = e->dest_lp;
  eventMsg.send_pe = e->send_pe;
  eventMsg.isAnti = 1;

  get_aggregator()->insertData(eventMsg, dest_peid);
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
    unsigned dest_peid = ((tw_lp*)e->src_lp)->type->chare_map(e->dest_lp);
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
