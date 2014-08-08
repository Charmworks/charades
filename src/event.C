#include "typedefs.h"
#include "event.h"
#include <stack>

std::stack<Event *> eventBuffers[128];

static inline tw_event * allocateEvent(int needMsg = 1) {
  Event * e = eventBuffers[CkMyPe()].pop();
  if(e == NULL) {
    e = new Event;
  }
  if(needMsg) {
    if(e->eventMsg == NULL) {
      e->eventMsg = new (PE_VALUE(g_tw_user_data_size)) RemoteMsg;
      e->userData = e->eventMsg->userData;
    }
  } else {
    if(e->eventMsg != NULL) {
      delete e->eventMsg;
    }
  }
  return e;
}

static inline void freeEvent(tw_event * e) {
  if(eventBuffers[CkMyPe()].size() >= PE_VALUE(g_tw_max_events_buffered)) {
    if(e->eventMsg) delete e->eventMsg;
    delete e;
  } else {
    e->state.owner = TW_event_null;
    eventBuffers[CkMyPe()].push(e);
  }
}

static inline void tw_free_output_messages(tw_event *e, int print_message)
{
  while (e->out_msgs) {
    tw_out *temp = e->out_msgs;
    if (print_message)
      printf("%s", temp->message);
    e->out_msgs = temp->next;
    // Put it back
    /* TODO : What is this for? Another buffer? */
    tw_kp_put_back_output_buffer(temp);
  }
}

static inline tw_event *
tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender) {
  tw_event	*e;
  tw_stime	recv_ts;

  recv_ts = sender->owner->now() + offset_ts;

  if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE)
  {
    if(offset_ts < PE_VALUE(g_tw_min_detected_offset))
      PE_VALUE(g_tw_min_detected_offset) = offset_ts;
  }

  /* If this event will be past the end time, or there
   * are no more free events available, use abort event.
   */
  /* TODO : do something about the abort events */
  if (recv_ts >= PE_VALUE(g_tw_ts_end)) {
    e = PE_VALUE(abort_event);
  } else {
    e = allocateEvent();
  }

  e->dest_lp = dest_gid;
  e->src_lp = (tw_lpid)sender;
  e->ts = recv_ts;

  tw_free_output_messages(e, 0);

  return e;
}

static inline void tw_event_free(tw_pe *pe, tw_event *e)
{
  tw_free_output_messages(e, 0);
  freeEvent(e);
}

static inline void link_causality (tw_event *nev, tw_event *cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}

void tw_event_send(tw_event * event) {
  tw_lp     *src_lp = (tw_lp*)event->src_lp;
  LPChare   *send_pe = src_lp->owner;
  int dest_peid;

  tw_stime   recv_ts = event->t;

  if (event == PE_VALUE(abort_event)) {
    if (recv_ts < PE_VALUE(g_tw_ts_end)) {
      send_pe->cev_abort = 1;
    }
    return;
  }

  //Trap lookahead violations in debug mode
  //Note that compiling with the -DNDEBUG flag will turn this off!
  if (PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    assert(recv_ts - send_pe->now() >= PE_VALUE(g_tw_lookahead) && "Lookahead violation: try decreasing the lookahead value");
  }

  if (event->out_msgs) {
    tw_error(TW_LOC, "It is an error to send an event with pre-loaded output message.");
  }

  link_causality(event, send_pe->currEvent);

  // call LP remote mapping function to get dest_pe
  dest_peid = src_lp->type->global_map(event->dest_lp);

  // fill in entries for remote msg
  e->eventMsg->event_id = e->event_id = e->owner->uniqID++;
  e->eventMsg->ts = e->ts;
  e->eventMsg->dest_lp = e->dest_lp;
  e->eventMsg->send_pe = e->send_pe = e->owner->thisIndex;

  lps(dest_peid).recv_event(e->eventMsg);
  e->state.owner = TW_sent;
  e->eventMsg = NULL;
}

static inline void event_cancel(tw_event * e) {
  /* already sent, send anti message and free me */
  if(event->state.owner == TW_sent) {
    LPChare *send_pe = ((tw_lp*)e->src_lp)->owner;
    RemoteMsg * eventMsg = new (0) RemoteMsg;
    eventMsg->isAnti = true;
    eventMsg->event_id = e->event_id;
    eventMsg->ts = e->ts;
    eventMsg->dest_lp = e->dest_lp;
    eventMsg->send_pe = e->send_pe;
    lps(dest_peid).recv_event(eventMsg);
    tw_event_free(send_pe, e);
    return;
  }

  LPChare *recv_pe = ((tw_lp*)e->dest_lp)->owner;
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
      recv_pe->cancel_q = e;
      return;

    default:
      tw_error(TW_LOC, "unknown fast local cancel owner %d", e->state.owner);
  }
  tw_error(TW_LOC, "Should be remote cancel!");
}

