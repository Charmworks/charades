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
  Time recv_ts = tw_now(sender) + offset_ts;

  Event* e;
  if (recv_ts >= g_tw_ts_end) {
    e = PE_VALUE(abort_event);
  } else {
    e = charm_allocate_event();
  }

  e->owner = sender;

  // TODO: How likely is incrementing uniqID here to cause overflow?
  e->ts = recv_ts;
  e->event_id = sender->owner->uniqID++;
  e->src_lp = sender->gid;
  e->dest_lp = dest_gid;

  return e;
}

void tw_event_free(Event *e, bool commit) {
  if(commit == true) {
    e->owner->commit(e->userData(), &e->cv);
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
            e->ts - tw_now(e->owner) >= g_tw_lookahead,
            "Lookahead violation: try decreasing the lookahead value");

  int dest_chare;
  link_causality(e, current_event(e->owner));
  dest_chare = g_chare_map(e->dest_lp);

  // The charm backend will fill in the remote event and send it
  int isRemote = charm_event_send(dest_chare, e);

  // Unless we are doing optimistic simulation we can just free the event
  if(isRemote && g_tw_synchronization_protocol != OPTIMISTIC) {
    charm_free_event(e);
  }
}

void tw_event_rollback(Event * event) {
  Event* e = event->caused_by_me;
  LPBase* dest_lp = event->owner;

  set_current_event(dest_lp, event);
  dest_lp->reverse(event->userData(), &event->cv);

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
  }
  return e;
}

void charm_free_event(Event* e) {
  // If we are optimistic, the event may need to be removed from the avl tree,
  // as well as freeing any events caused by it.
  if (g_tw_synchronization_protocol == OPTIMISTIC) {
    if(e->state.avl_tree == 1) {
      avlDelete(&(e->owner)->owner->all_events, e);
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
int charm_event_send(unsigned dest_chare_id, Event * e) {
  static Scheduler* scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
  LPChare* send_chare = (e->owner)->owner;
  LPChare* dest_chare;

  // Check to see if this is a local send or not.
  if (dest_chare_id == send_chare->thisIndex) {
    dest_chare = send_chare;
  } else {
    dest_chare = NULL;
  }

  // If we got a pointer, do a local send. Otherwise, send remotely.
  if (dest_chare != NULL) {
    // Check if an LP is sending to an LP other than itself (for stats).
    if (e->dest_lp == e->src_lp) {
      PE_STATS(self_sends)++;
    } else {
      PE_STATS(local_sends)++;
    }
    dest_chare->recv_local_event(e);
    return 0;
  } else {
    PE_STATS(remote_sends)++;
    // Fill the fields of the charm message to prepare it for sending.
    e->eventMsg->ts = e->ts;
    e->eventMsg->event_id = e->event_id;
    e->eventMsg->src_lp = e->src_lp;
    e->eventMsg->dest_lp = e->dest_lp;

    scheduler->produce(e->eventMsg);
    lps(dest_chare_id).recv_remote_event(e->eventMsg);
    e->state.owner = TW_sent;
    e->eventMsg = NULL;
    return 1;
  }
}

// Allocate a new remote message, fill it based on e, and send it.
// An anti send will never be to a local chare, because locally sent events
// will never have the owner set to TW_sent.
void charm_anti_send(unsigned dest_chare_id, Event * e) {
  static Scheduler* scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
  RemoteEvent * eventMsg = PE_VALUE(event_buffer)->get_remote_event();
  eventMsg->event_id = e->event_id;
  eventMsg->ts = e->ts;
  eventMsg->dest_lp = e->dest_lp;
  eventMsg->src_lp = e->src_lp;

  scheduler->produce(eventMsg);
  PE_STATS(anti_sends)++;
  lps(dest_chare_id).recv_anti_event(eventMsg);
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
    unsigned dest_chare_id = g_chare_map(e->dest_lp);
    charm_anti_send(dest_chare_id, e);

    tw_event_free(e,false);
    return;
  }

  // If is local, then the LP can cancel it
  e->owner->owner->cancel_event(e);
}

#include "event.def.h"
