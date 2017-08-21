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

Event* event_alloc() {
  return PE_VALUE(event_buffer)->get_event();
};

Event* event_alloc(RemoteEvent* msg, uint64_t dest_gid, Time offset, LPBase * sender) {
  msg->ts = sender->get_current_time() + offset;
  msg->event_id = sender->owner->get_next_event_id();
  msg->src_lp = sender->gid;
  msg->dest_lp = dest_gid;

  Event* e;
  if (msg->ts >= g_tw_ts_end) {
    e = PE_VALUE(abort_event);
  } else {
    e = event_alloc();
  }

  e->owner = sender;
  e->set_msg(msg);

  return e;
}

void tw_event_free(Event *e, bool commit) {
  if(commit == true) {
    e->owner->commit(e);
    PE_STATS(events_committed)++;
  }
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
        tw_event_free(event, false);
      }
      event = n;
    }
  }

  delete e->msg;
  e->msg = NULL;
  PE_VALUE(event_buffer)->free_event(e);
}

void tw_event_send(Event * e) {
  static Scheduler* scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
  if (e == PE_VALUE(abort_event)) {
    TW_ASSERT(e->ts >= g_tw_ts_end, "Can't send abort event before end\n");
    return;
  }
  TW_ASSERT(g_tw_synchronization_protocol != CONSERVATIVE ||
            e->ts - e->owner->get_current_time() >= g_tw_lookahead,
            "Lookahead violation: try decreasing the lookahead value");

  link_causality(e, e->owner->get_current_event());

  LPChare* send_chare = (e->owner)->owner;
  int dest_chare_id = g_chare_map(e->dest_lp);
  LPChare* dest_chare;
  if (dest_chare_id == send_chare->thisIndex) {
    dest_chare = send_chare;
  } else {
    dest_chare = NULL;
  }

  if (dest_chare != NULL) {
    // Check if an LP is sending to an LP other than itself (for stats).
    if (e->dest_lp == e->src_lp) {
      PE_STATS(self_sends)++;
    } else {
      PE_STATS(local_sends)++;
    }
    dest_chare->recv_local_event(e);
  } else {
    PE_STATS(remote_sends)++;
    scheduler->produce(e->get_msg());
    lps(dest_chare_id).recv_remote_event(e->get_msg());
    e->msg = NULL;
    if (g_tw_synchronization_protocol == OPTIMISTIC) {
      e->state.owner = TW_sent;
    } else {
      tw_event_free(e, false);
    }
  }
}

void tw_event_rollback(Event * event) {
  Event* e = event->caused_by_me;
  LPBase* dest_lp = event->owner;

  dest_lp->set_current_event(event);
  dest_lp->reverse(event);

  while (e) {
    Event *n = e->cause_next;
    e->cause_next = NULL;

    charm_event_cancel(e);
    e = n;
  }

  event->caused_by_me = NULL;

  PE_STATS(events_rolled_back)++;
}

// Allocate a new remote message, fill it based on e, and send it.
// An anti send will never be to a local chare, because locally sent events
// will never have the owner set to TW_sent.
void charm_anti_send(unsigned dest_chare_id, Event* e) {
  static Scheduler* scheduler = (Scheduler*)CkLocalBranch(scheduler_id);
  RemoteEvent* eventMsg = new (0) RemoteEvent();
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
