#include "event.h"

#include "avl_tree.h"
#include "event_buffer.h"
#include "globals.h"
#include "lp.h"
#include "scheduler.h"
#include "statistics.h"
#include "typedefs.h"
#include "util.h"

extern CProxy_LPChare lps;
extern CProxy_Scheduler scheduler;

// Public functions exposed to the user for allocating, sending, freeing, and
// rolling back events.
Event* tw_event_new(LPID dest_gid, Time offset_ts, LPBase * sender) {
  Event* e;
  Time recv_ts = tw_now(sender) + offset_ts;

  if (recv_ts >= g_tw_ts_end) {
    e = PE_VALUE(abort_event);
  } else {
    e = charm_allocate_event();
  }

  e->dest_lp = dest_gid;
  e->src_lp = (LPID)sender;
  e->ts = recv_ts;
  e->send_pe = (uint64_t)(sender->owner);

  return e;
}

void tw_event_free(Event *e, bool commit) {
  if(commit == true) {
    LPBase* lp;
    lp = (LPBase*)e->dest_lp;
    lp->commit(tw_event_data(e), &e->cv);
    PE_STATS(events_committed)++;
  }
  charm_free_event(e);
}

void tw_event_send(Event * e) {
  if (e == PE_VALUE(abort_event)) {
    TW_ASSERT(e->ts >= g_tw_ts_end, "Can't send abort event before end\n");
    return;
  }
  TW_ASSERT(g_tw_synchronization_protocol != CONSERVATIVE ||
            e->ts - tw_now((LPBase*)e->src_lp) >= g_tw_lookahead,
            "Lookahead violation: try decreasing the lookahead value");

  int dest_peid;
  LPBase* src_lp = (LPBase*)e->src_lp;
  link_causality(e, current_event(src_lp));
  dest_peid = g_chare_map(e->dest_lp);

  // The charm backend will fill in the remote event and send it
  int isRemote = charm_event_send(dest_peid, e);

  // Unless we are doing optimistic simulation we can just free the event
  if(isRemote && g_tw_synchronization_protocol != OPTIMISTIC) {
    charm_free_event(e);
  }
}

void tw_event_rollback(Event * event) {
  Event  *e = event->caused_by_me;
  LPBase     *dest_lp = (LPBase*)event->dest_lp;

  set_current_event(dest_lp, event);
  dest_lp->reverse(tw_event_data(event), &event->cv);

  while (e) {
    Event *n = e->cause_next;
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
      avlDelete(&((LPBase*)e->dest_lp)->owner->all_events, e);
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
  LPChare* send_pe = (LPChare*)(e->send_pe);
  LPChare* dest_pe;

  // When e is passed in, src_lp and send_pe are pointers, dest_lp is a gid.
  e->src_lp = ((LPBase*)e->src_lp)->gid;
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
  LPChare *dest_pe = ((LPBase*)e->dest_lp)->owner;
  dest_pe->cancel_event(e);
}

#include "event.def.h"
