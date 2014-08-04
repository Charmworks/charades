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

void tw_event_send(tw_event * event) {
  tw_lp     *src_lp = (tw_lp*)event->src_lp;
  LPChare   *send_pe = src_lp->owner;
  int dest_peid;

  tw_stime   recv_ts = event->ts;

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
  e->eventMsg->event_id = e->owner->uniqID++;
  e->eventMsg->ts = e->ts;
  e->eventMsg->dest_lp = e->dest_lp;
  e->eventMsg->send_pe = e->owner->thisIndex;

  lps(dest_lp).recv_event(e->eventMsg);
  e->eventMsg = NULL;
}

//TODO continue here
static inline void event_cancel(tw_event * event) {
    LPChare *send_pe = event->src_lp->owner;
    tw_peid dest_peid;

    if(event->state.owner == TW_net_asend || event->state.owner == TW_pe_sevent_q) {
        /* Slowest approach of all; this has to be sent over the
        * network to let the dest_pe know it shouldn't have seen
        * it in the first place.
        */
        tw_net_cancel(event);

        if(tw_gvt_inprogress(send_pe)) {
            send_pe->trans_msg_ts = min(send_pe->trans_msg_ts, event->recv_ts);
        }

        return;
    }

    dest_peid = event->dest_lp->pe->id;

    if (send_pe->id == dest_peid) {
        switch (event->state.owner) {
            case TW_pe_pq:
                /* Currently in our pq and not processed; delete it and
                * free the event buffer immediately.  No need to wait.
                */
                tw_pq_delete_any(send_pe->pq, event);
                tw_event_free(send_pe, event);
                break;

            case TW_pe_event_q:
            case TW_kp_pevent_q:
                local_cancel(send_pe, event);

                if(tw_gvt_inprogress(send_pe)) {
                    send_pe->trans_msg_ts = min(send_pe->trans_msg_ts, event->recv_ts);
                }
                break;

            default:
                tw_error(TW_LOC, "unknown fast local cancel owner %d", event->state.owner);
        }
    } else if (send_pe->node == dest_peid) {
        /* Slower, but still a local cancel, so put into
        * top of dest_pe->cancel_q for final deletion.
        */
        local_cancel(event->dest_lp->pe, event);
        send_pe->stats.s_nsend_loc_remote--;

        if(tw_gvt_inprogress(send_pe)) {
            send_pe->trans_msg_ts = min(send_pe->trans_msg_ts, event->recv_ts);
        }
    } else {
        tw_error(TW_LOC, "Should be remote cancel!");
    }
}

