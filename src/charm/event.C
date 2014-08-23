#include "typedefs.h"
#include "globals.h"
#include "event.h"
#include "lp.h"
#include "pe.h"
#include "lp_struct.h"
#include "ross_util.h"
#include "avl_tree.h"
#include "ross_api.h"
#include <assert.h>
#include <stack>

extern CProxy_LP lps;
extern CProxy_PE pes;

tw_event * allocateEvent(int needMsg = 1) {
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

tw_out* allocate_output_buffer() {
  tw_out* free_buf = NULL;
  if(PE_VALUE(output)) {
    free_buf = PE_VALUE(output);
    PE_VALUE(output) = free_buf->next;
    free_buf->next = NULL;
  }
  return free_buf;
}

static inline void freeEvent(tw_event * e) {
  if(e->state.remote == 1) {
    avlDelete(&((LPStruct*)e->dest_lp)->owner->all_events, e);
  }
  if(PE_VALUE(eventBuffer).size() >= PE_VALUE(g_tw_max_events_buffered)) {
    if(e->eventMsg) delete e->eventMsg;
    delete e;
  } else {
    e->state.remote = 0;
    e->state.owner = TW_event_null;
    e->caused_by_me = NULL;
    e->cause_next = NULL;
    e->cancel_next = NULL;
    PE_VALUE(eventBuffer).push(e);
  }
}

inline void free_output_buffer(tw_out *buffer) {
  buffer->next = PE_VALUE(output);
  PE_VALUE(output) = buffer;
}

static inline void tw_free_output_messages(tw_event *e, int print_message)
{
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

tw_event * tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender) {
  tw_event	*e;
  tw_stime	recv_ts;

  recv_ts = sender->owner->current_time + offset_ts;

  if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE)
  {
    if(offset_ts < PE_VALUE(g_tw_min_detected_offset))
      PE_VALUE(g_tw_min_detected_offset) = offset_ts;
  }

  if (recv_ts >= PE_VALUE(g_tw_ts_end)) {
    e = PE_VALUE(abort_event);
  } else {
    e = allocateEvent();
  }

  e->dest_lp = dest_gid;
  e->src_lp = (tw_lpid)sender;
  e->ts = recv_ts;
  e->send_pe = (tw_peid)sender->owner;

  tw_free_output_messages(e, 0);

  return e;
}

void tw_event_free(tw_pe *pe, tw_event *e)
{
  tw_free_output_messages(e, 0);
  freeEvent(e);
}

static inline void link_causality (tw_event *nev, tw_event *cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}

void tw_event_send(tw_event * e) {
  tw_lp     *src_lp = (tw_lp*)e->src_lp;
  LP        *send_pe = src_lp->owner;
  int dest_peid;

  tw_stime   recv_ts = e->ts;
  DEBUG2("[%d] Sending event to %d at %lf \n", CkMyPe(), e->dest_lp, recv_ts);

  if (e == PE_VALUE(abort_event)) {
    /* TODO (nikhil): Handle case where abort event is caused by lack of memory */
    //if (recv_ts < PE_VALUE(g_tw_ts_end)) {
      //send_pe->cev_abort = 1;
    //}
    return;
  }

  //Trap lookahead violations in debug mode
  if (PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    assert(recv_ts - send_pe->current_time >= PE_VALUE(g_tw_lookahead) && "Lookahead violation: try decreasing the lookahead value");
  }

  if (e->out_msgs) {
    tw_error(TW_LOC, "It is an error to send an event with pre-loaded output message.");
  }

  link_causality(e, send_pe->currEvent);

  // call LP remote mapping function to get dest_pe
  dest_peid = src_lp->type->chare_map(e->dest_lp);

  // fill in entries for remote msg
  // TODO (nikhil): Why are these set here and not at event creation?
  e->eventMsg->event_id = e->event_id = ((LP*)(e->send_pe))->uniqID++;
  e->eventMsg->ts = e->ts;
  e->eventMsg->dest_lp = e->dest_lp;
  e->eventMsg->send_pe = e->send_pe = ((LP*)(e->send_pe))->thisIndex;

  lps(dest_peid).recv_event(e->eventMsg);
  e->state.owner = TW_sent;
  e->eventMsg = NULL;
  if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
   freeEvent(e);
  }
}

void event_cancel(tw_event * e) {
  /* already sent, send anti message and free me */
  if(e->state.owner == TW_sent) {
    LP *send_pe = ((tw_lp*)e->src_lp)->owner;
    RemoteEvent * eventMsg = new (0) RemoteEvent;
    eventMsg->isAnti = true;
    eventMsg->event_id = e->event_id;
    eventMsg->ts = e->ts;
    eventMsg->dest_lp = e->dest_lp;
    eventMsg->send_pe = e->send_pe;
    lps(((tw_lp*)(e->src_lp))->type->chare_map(e->dest_lp)).recv_event(eventMsg);
    tw_event_free(send_pe, e);
    return;
  }

  LP *recv_pe = ((tw_lp*)e->dest_lp)->owner;
  switch (e->state.owner) {
    case TW_chare_q:
      /* Currently in our pq and not processed; delete it and
       * free the event buffer immediately.  No need to wait.
       */
      recv_pe->delete_pending(e);
      tw_event_free(recv_pe, e);
      return;

    case TW_rollback_q:
      e->cancel_next = recv_pe->cancel_q;
      if(recv_pe->cancel_q_end == NULL) {
        recv_pe->cancel_q_end = e;
      }
      recv_pe->cancel_q = e;
      if(!recv_pe->enqueued_cancel_q) {
        pes.ckLocalBranch()->cancel_q.push_back(recv_pe);
        recv_pe->enqueued_cancel_q = true;
      }
      return;

    default:
      tw_error(TW_LOC, "unknown fast local cancel owner %d", e->state.owner);
  }
  tw_error(TW_LOC, "Should be remote cancel!");
}

void tw_event_rollback(tw_event * event) {
  tw_event  *e = event->caused_by_me;
  tw_lp     *dest_lp = (tw_lp*)event->dest_lp;

  tw_free_output_messages(event, 0);

  dest_lp->owner->currEvent = event;
  dest_lp->owner->current_time = event->ts;
  dest_lp->type->reverse(dest_lp->state, &event->cv, tw_event_data(event), dest_lp);

  while (e) {
    tw_event *n = e->cause_next;
    e->cause_next = NULL;

    event_cancel(e);
    e = n;
  }

  event->caused_by_me = NULL;
}

#include "event.def.h"
