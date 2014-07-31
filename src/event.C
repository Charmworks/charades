#include "typedefs.h"
#include "event.h"
#include <stack>

std::stack<Event *> eventBuffers[128];

static inline tw_event * allocateEvent() {
  Event * e = eventBuffers[CkMyPe()].pop();
  if(e == NULL) {
    e = new Event;
    e->userData = new char[PE_VALUE(g_tw_user_data_size)];
  }
  return e;
}

static inline void freeEvent(tw_event * e) {
  if(e->remoteMsg) delete e->remoteMsg;
  if(eventBuffers[CkMyPe()].size() >= PE_VALUE(g_tw_max_events_buffered)) {
    delete [] e->userData;
    delete e;
  } else {
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

  /* TODO: use this info */
  /*if(g_tw_synchronization_protocol == CONSERVATIVE)
    {
    if(offset_ts < g_tw_min_detected_offset)
    g_tw_min_detected_offset = offset_ts;
    }*/

  /* If this event will be past the end time, or there
   * are no more free events available, use abort event.
   */
  /* TODO : do something among the abort events */
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

/* TODO TODO Continue here nex time */
void tw_event_send(tw_event * event) {
  tw_lp     *src_lp = (tw_lp*)event->src_lp;
  tw_pe     *send_pe = src_lp->pe;
  tw_pe     *dest_pe = NULL;

  tw_peid        dest_peid = -1;
  tw_stime   recv_ts = event->recv_ts;

  if (event == send_pe->abort_event) {
    if (recv_ts < g_tw_ts_end) {
      send_pe->cev_abort = 1;
    }
    return;
  }

  //Trap lookahead violations in debug mode
  //Note that compiling with the -DNDEBUG flag will turn this off!
  if (g_tw_synchronization_protocol == CONSERVATIVE) {
    assert(recv_ts - tw_now(src_lp) >= g_tw_lookahead && "Lookahead violation: try decreasing the lookahead value");
  }

  if (event->out_msgs) {
    tw_error(TW_LOC, "It is an error to send an event with pre-loaded output message.");
  }

  link_causality(event, send_pe->cur_event);

  // call LP remote mapping function to get dest_pe
  dest_peid = (*src_lp->type->map) ((tw_lpid) event->dest_lp);

  if (dest_peid == g_tw_mynode) {
    event->dest_lp = tw_getlocal_lp((tw_lpid) event->dest_lp);
    dest_pe = event->dest_lp->pe;

    if (send_pe == dest_pe && event->dest_lp->kp->last_time <= recv_ts) {
      /* Fast case, we are sending to our own PE and there is
       * no rollback caused by this send.  We cannot have any
       * transient messages on local sends so we can return.
       */
      tw_pq_enqueue(send_pe->pq, event);
      return;
    } else {
      /* Slower, but still local send, so put into top of
       * dest_pe->event_q.
       */

