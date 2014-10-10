#include "event.h"

#include "lp.h"
#include "pe.h"

#include "typedefs.h"
#include "globals.h"
#include "lp_struct.h"
#include "avl_tree.h"

#include "ross_util.h"

#include <assert.h>
#include <stack>

extern CProxy_LP lps;
extern CProxy_PE pes;

tw_event * charm_allocate_event(int needMsg = 1) {
  Event *e;
  if(PE_VALUE(eventBuffer).size() != 0) {
    e = PE_VALUE(eventBuffer).top();
    PE_VALUE(eventBuffer).pop();
  } else {
    e = new Event;
  }
  if(needMsg) {
    if(e->eventMsg == NULL) {
      e->eventMsg = new (PE_VALUE(g_tw_msg_sz)) RemoteEvent;
      e->userData = e->eventMsg->userData;
    }
  } else {
    if(e->eventMsg != NULL) {
      delete e->eventMsg;
      e->eventMsg = NULL;
    }
  }
  return e;
}

void charm_free_event(tw_event * e) {
  if (PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
    if(e->state.remote == 1) {
      DEBUG3("Delete %d %d %lf \n",e->send_pe, e->event_id, e->ts);
      avlDelete(&((LPStruct*)e->dest_lp)->owner->all_events, e);
    }

    tw_event  *event = e->caused_by_me;
    while (event) {
      tw_event *n = event->cause_next;
      charm_free_event(event);
      event = n;
    }
  }

  // TODO: Once allocation is handled correctly we shouldn't need this if
  if(PE_VALUE(eventBuffer).size() >= PE_VALUE(g_tw_max_events_buffered)) {
    if(e->eventMsg) delete e->eventMsg;
    delete e;
  } else {
    e->state.remote = 0;
    e->state.cancel_q = 0;
    e->state.owner = TW_event_null;
    e->caused_by_me = NULL;
    e->cause_next = NULL;
    e->cancel_next = NULL;
    PE_VALUE(eventBuffer).push(e);
  }
}

// Fill and send a regular message, possibly using short-circuiting
void charm_event_send(unsigned dest_peid, tw_event * e) {
  LP *send_pe = (LP*)(e->send_pe);
  LP *dest_pe;

  // Fill the fields of the charm message to prepare it for sending
  e->eventMsg->event_id = e->event_id = ((LP*)(e->send_pe))->uniqID++;
  e->eventMsg->ts = e->ts;
  e->eventMsg->dest_lp = e->dest_lp;
  e->eventMsg->send_pe = e->send_pe = ((LP*)(e->send_pe))->thisIndex;

  // Check for possible short-circuiting and send the message
  if (dest_peid == send_pe->thisIndex) {
    send_pe->recv_event(e->eventMsg);
  } else if ((dest_pe = lps(dest_peid).ckLocal()) != NULL &&
      e->ts > dest_pe->current_time) {
    dest_pe->recv_event(e->eventMsg);
  } else {
    lps(dest_peid).recv_event(e->eventMsg);
  }
}

// Fill and send an anti-message to cancel e
void charm_anti_send(unsigned dest_peid, tw_event * e) {
  RemoteEvent * eventMsg = new (0) RemoteEvent;
  eventMsg->isAnti = true;
  eventMsg->event_id = e->event_id;
  eventMsg->ts = e->ts;
  eventMsg->dest_lp = e->dest_lp;
  eventMsg->send_pe = e->send_pe;

  // TODO: Also include short-circuit logic here
  lps(dest_peid).recv_event(eventMsg);
}

// TODO: Maybe split this up into it's charm and ross components
// TODO: Could be cleaned up further. More logic can/should be pushed into LP
void charm_event_cancel(tw_event * e) {
  //already in cancel q, return
  if(e->state.cancel_q) {
    return;
  }

  // If already sent, populate and send an anti-message
  if(e->state.owner == TW_sent) {
    unsigned dest_peid = ((tw_lp*)e->src_lp)->type->chare_map(e->dest_lp);
    LP *send_pe = (LP*)e->send_pe;
    charm_anti_send(dest_peid, e);
    tw_event_free(send_pe, e);
    return;
  }

  // If not already sent, find the owner and cancel appropriately
  LP *recv_pe = ((tw_lp*)e->dest_lp)->owner;
  switch (e->state.owner) {
    case TW_chare_q:
      recv_pe->delete_pending(e);
      tw_event_free(recv_pe, e);
      return;

    case TW_rollback_q:
      e->cancel_next = recv_pe->cancel_q;
      if(e->ts < recv_pe->min_cancel_q) {
        recv_pe->min_cancel_q = e->ts;
      }
      recv_pe->cancel_q = e;
      if(!recv_pe->enqueued_cancel_q) {
        // TODO: This should be moved to the LP so PE is cached (also remove pe.h from includes)
        pes.ckLocalBranch()->cancel_q.push_back(recv_pe);
        recv_pe->enqueued_cancel_q = true;
      }
      return;

    default:
      tw_error(TW_LOC,
          "unknown fast local cancel owner %d at %lf", e->state.owner, e->ts);
  }
  tw_error(TW_LOC, "Should be remote cancel!");
}

#include "event.def.h"
