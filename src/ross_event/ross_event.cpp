#include "ross_event.h"
#include "ross_util.h"

#include "lp_struct.h"
#include "charm_functions.h"
#include "globals.h"

#include <assert.h>

tw_event * tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender) {
  tw_event	*e;
  tw_stime	recv_ts;

  recv_ts = tw_now(sender) + offset_ts;

  if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    if(offset_ts < PE_VALUE(g_tw_min_detected_offset)) {
      PE_VALUE(g_tw_min_detected_offset) = offset_ts;
    }
  }

  if (recv_ts >= PE_VALUE(g_tw_ts_end)) {
    e = PE_VALUE(abort_event);
  } else {
    e = allocateEvent();
  }

  e->dest_lp = dest_gid;
  e->src_lp = (tw_lpid)sender;
  e->ts = recv_ts;
  e->send_pe = (tw_peid)(sender->owner);

  // TODO: Why is this needed?
  tw_free_output_messages(e, 0);

  return e;
}

void tw_event_send(tw_event * e) {
  tw_lp     *src_lp = (tw_lp*)e->src_lp;
  int dest_peid;

  DEBUG2("[%d] Sending event to %d at %lf \n", CkMyPe(), e->dest_lp, recv_ts);

  if (e == PE_VALUE(abort_event)) {
    // TODO: Handle case where abort event is caused by lack of memory
    //if (recv_ts < PE_VALUE(g_tw_ts_end)) {
      //send_pe->cev_abort = 1;
    //}
    return;
  }

  //Trap lookahead violations in debug mode
  if (PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    assert(e->ts - tw_now(src_lp) >= PE_VALUE(g_tw_lookahead) &&
        "Lookahead violation: try decreasing the lookahead value");
  }

  if (e->out_msgs) {
    tw_error(TW_LOC,
        "It is an error to send an event with pre-loaded output message.");
  }

  link_causality(e, tw_current_event(src_lp));

  // Call LP remote mapping function to get dest_pe
  dest_peid = src_lp->type->chare_map(e->dest_lp);

  // The charm backend will fill in the remote event and send it
  charm_event_send(dest_peid, e); 

  // Deallocate the event or unlink the event message from the event
  // depending on the synchronization protocol
  if(PE_VALUE(g_tw_synchronization_protocol) != OPTIMISTIC) {
    freeEvent(e);
  } else {
    e->state.owner = TW_sent;
    e->eventMsg = NULL;
  }
}

static inline void link_causality (tw_event *nev, tw_event *cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}

static inline void freeEvent(tw_event * e) {
  if (PE_VALUE(g_tw_synchronization_protocol) == OPTIMISTIC) {
    if(e->state.remote == 1) {
      DEBUG3("Delete %d %d %lf \n",e->send_pe, e->event_id, e->ts);
      avlDelete(&((LPStruct*)e->dest_lp)->owner->all_events, e);
    }

    tw_event  *event = e->caused_by_me;
    while (event) {
      tw_event *n = event->cause_next;
      freeEvent(event);
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


void event_cancel(tw_event * e) {
  //already in cancel q, return
  if(e->state.cancel_q) {
    return;
  }

  // If already sent, populate and send an anti-message
  if(e->state.owner == TW_sent) {
    charm_anti_send(e);
    tw_event_free(send_pe, e);
    return;
  }

  // TODO: The following cases can be handled by the LP chares
  //LP *recv_pe = ((tw_lp*)e->dest_lp)->owner;
  switch (e->state.owner) {
    case TW_chare_q:
      /* Currently in our pq and not processed; delete it and
       * free the event buffer immediately.  No need to wait.
       */
      //recv_pe->delete_pending(e);
      charm_delete_pending(e);
      tw_event_free(recv_pe, e);
      return;

    case TW_rollback_q:
      charm_add_to_cancel_q(e);
      /*e->cancel_next = recv_pe->cancel_q;
      if(e->ts < recv_pe->min_cancel_q) {
        recv_pe->min_cancel_q = e->ts;
      }
      recv_pe->cancel_q = e;
      if(!recv_pe->enqueued_cancel_q) {
        pes.ckLocalBranch()->cancel_q.push_back(recv_pe);
        recv_pe->enqueued_cancel_q = true;
      }*/
      return;

    default:
      tw_error(TW_LOC, "unknown fast local cancel owner %d at %lf", e->state.owner, e->ts);
  }
  tw_error(TW_LOC, "Should be remote cancel!");
}

static inline void tw_free_output_messages(tw_event *e, int print_message) {
  while (e->out_msgs) {
    tw_out *temp = e->out_msgs;
    if (print_message)
      printf("%s", temp->message);
    e->out_msgs = temp->next;
    // Put it back
    free_output_buffer(temp);
  }
  e->out_msgs = NULL;
}

void tw_event_free(tw_pe *pe, tw_event *e) {
  tw_free_output_messages(e, 0);
  DEBUG2("Free event %d %d %lf\n", e->send_pe, e->event_id, e->ts);
  freeEvent(e);
}

void tw_event_rollback(tw_event * event) {
  tw_event  *e = event->caused_by_me;
  tw_lp     *dest_lp = (tw_lp*)event->dest_lp;

  tw_free_output_messages(event, 0);

  // TODO: We only need to set these in one place
  //dest_lp->owner->currEvent = event;
  //dest_lp->owner->current_time = event->ts;
  dest_lp->type->reverse(dest_lp->state, &event->cv, tw_event_data(event), dest_lp);
  (PE_VALUE(netEvents))--;

  while (e) {
    tw_event *n = e->cause_next;
    e->cause_next = NULL;

    event_cancel(e);
    e = n;
  }

  event->caused_by_me = NULL;
}
