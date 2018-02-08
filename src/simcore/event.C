#include "event.h"

#include "avl_tree.h"
#include "event_buffer.h"
#include "globals.h"
#include "lp.h"
#include "scheduler.h"
#include "statistics.h"
#include "typedefs.h"
#include "util.h"

extern CProxy_LP lps;
extern CProxy_Scheduler scheduler;

// Public functions exposed to the user for allocating, sending, freeing, and
// rolling back events.
tw_event * tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender) {
  tw_event	*e;
  tw_stime	recv_ts;

  recv_ts = tw_now(sender) + offset_ts;

  if (recv_ts >= g_tw_ts_end) {
    e = PE_VALUE(abort_event);
  } else {
    e = charm_allocate_event();
  }

  e->dest_lp = dest_gid;
  e->src_lp = (tw_lpid)sender;
  e->ts = recv_ts;
  e->send_pe = (tw_peid)(sender->owner);

  return e;
}

void tw_event_free(tw_event *e, bool commit) {
  if(commit == true) {
    LPStruct * lp;
    lp = (LPStruct*)e->dest_lp;
    if (lp->type->commit != NULL) {
      lp->type->commit(lp->state, &e->cv, tw_event_data(e), lp);
    }
    PE_STATS(events_committed)++;
  }
  charm_free_event(e);
}

void tw_event_send(tw_event * e) {
  if (e == PE_VALUE(abort_event)) {
    TW_ASSERT(e->ts > g_tw_ts_end, "Can't send abort event before end\n");
    return;
  }
  TW_ASSERT(g_tw_synchronization_protocol != CONSERVATIVE ||
            e->ts - tw_now((tw_lp*)e->src_lp) >= g_tw_lookahead,
            "Lookahead violation: try decreasing the lookahead value");

  int dest_peid;
  tw_lp* src_lp = (tw_lp*)e->src_lp;
  link_causality(e, current_event(src_lp));
  dest_peid = g_chare_map(e->dest_lp);

  // The charm backend will fill in the remote event and send it
  int isRemote = charm_event_send(dest_peid, e);

  // Unless we are doing optimistic simulation we can just free the event
  if(isRemote && g_tw_synchronization_protocol != OPTIMISTIC) {
    charm_free_event(e);
  }
}

void tw_event_rollback(tw_event * event) {
  tw_event  *e = event->caused_by_me;
  tw_lp     *dest_lp = (tw_lp*)event->dest_lp;

  set_current_event(dest_lp, event);
  dest_lp->type->revent(dest_lp->state, &event->cv, tw_event_data(event), dest_lp);

  while (e) {
    tw_event *n = e->cause_next;
    e->cause_next = NULL;

    charm_event_cancel(e);
    e = n;
  }

  event->caused_by_me = NULL;

  PE_STATS(events_rolled_back)++;
}

Event* charm_allocate_event(int needMsg) {
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
  static Scheduler* scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
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
    if (e->dest_lp == e->src_lp) {
      PE_STATS(self_sends)++;
    } else {
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

    scheduler->produce(e->eventMsg);
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
  static Scheduler* scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
  RemoteEvent * eventMsg = PE_VALUE(event_buffer)->get_remote_event();
  eventMsg->event_id = e->event_id;
  eventMsg->ts = e->ts;
  eventMsg->dest_lp = e->dest_lp;
  eventMsg->send_pe = e->send_pe;

  scheduler->produce(eventMsg);
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
