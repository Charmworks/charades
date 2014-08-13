#include "typedefs.h"
#include "globals.h"
#include "event.h"
#include "lp.h"
#include "pe.h"
#include "lp_struct.h"
#include <assert.h>
#include <stack>

extern CProxy_LP lps;
extern CProxy_PE pes;

#ifndef NO_FORWARD_DECLS
void tw_error(const char* file, int line, const char* fmt, ...);
#endif

// TODO: This should go in a better place
static const unsigned CONSERVATIVE=2;

// TODO: This is just here so this compiles
typedef LP tw_pe;

std::stack<Event *> eventBuffers[128];
CkpvDeclare(tw_out*, output);

static inline tw_event * allocateEvent(int needMsg = 1) {
  Event * e = eventBuffers[CkMyPe()].top();
  eventBuffers[CkMyPe()].pop();
  if(e == NULL) {
    e = new Event;
  }
  if(needMsg) {
    if(e->eventMsg == NULL) {
      e->eventMsg = new (PE_VALUE(g_tw_user_data_size)) RemoteEvent;
      e->userData = e->eventMsg->userData;
    }
  } else {
    if(e->eventMsg != NULL) {
      delete e->eventMsg;
    }
  }
  return e;
}

inline tw_out* allocate_output_buffer() {
  tw_out* free_buf = NULL;
  if(CkpvAccess(output)) {
    free_buf = CkpvAccess(output);
    CkpvAccess(output) = free_buf->next;
  }
  return free_buf;
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

inline void free_output_buffer(tw_out *buffer) {
  buffer->next = CkpvAccess(output);
  CkpvAccess(output) = buffer;
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
}

tw_event * tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender) {
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

void tw_event_send(tw_event * e) {
  tw_lp     *src_lp = (tw_lp*)e->src_lp;
// TODO: How do we deal with "pes"
  LP       *send_pe = src_lp->owner;
  int dest_peid;

  tw_stime   recv_ts = e->ts;

  if (e == PE_VALUE(abort_event)) {
    if (recv_ts < PE_VALUE(g_tw_ts_end)) {
      // TODO: Don't know what this is
      //send_pe->cev_abort = 1;
    }
    return;
  }

  //Trap lookahead violations in debug mode
  //Note that compiling with the -DNDEBUG flag will turn this off!
  if (PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE) {
    assert(recv_ts - send_pe->now() >= PE_VALUE(g_tw_lookahead) && "Lookahead violation: try decreasing the lookahead value");
  }

  if (e->out_msgs) {
    tw_error(TW_LOC, "It is an error to send an event with pre-loaded output message.");
  }

  link_causality(e, send_pe->currEvent);

  // call LP remote mapping function to get dest_pe
  dest_peid = src_lp->type->global_map(e->dest_lp);

  // fill in entries for remote msg
  // TODO: Owner needs to be figured out
  //e->eventMsg->event_id = e->event_id = e->owner->uniqID++;
  e->eventMsg->ts = e->ts;
  e->eventMsg->dest_lp = e->dest_lp;
  // TODO: PE needs to be figured out
  //e->eventMsg->send_pe = e->send_pe = e->owner->thisIndex;

  lps(dest_peid).recv_event(e->eventMsg);
  e->state.owner = TW_sent;
  e->eventMsg = NULL;
}

static inline void event_cancel(tw_event * e) {
  /* already sent, send anti message and free me */
  if(e->state.owner == TW_sent) {
    LP *send_pe = ((tw_lp*)e->src_lp)->owner;
    RemoteEvent * eventMsg = new (0) RemoteEvent;
    eventMsg->isAnti = true;
    eventMsg->event_id = e->event_id;
    eventMsg->ts = e->ts;
    eventMsg->dest_lp = e->dest_lp;
// TODO: Haven't decided what to do with pe yet
    //eventMsg->send_pe = e->send_pe;
    // TODO: Where does dest_peid come from?
    //lps(dest_peid).recv_event(eventMsg);
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
      recv_pe->cancel_q = e;
      if(!recv_pe->enqueued_cancel_q) {
        pes.ckLocalBranch()->cancel_q.push_back(recv_pe);
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
  // TODO: I think this is handled in the LP. Need to find out.
  //dest_lp->owner->current_time = event->ts;
  //(*dest_lp->type->revent)(dest_lp->cur_state, &event->cv, tw_event_data(event), dest_lp);
  /* TODO talk to Eric and fix this */
  //LPStruct *lp = &lp_structs[e->local_id];
  //dest_lp->type->reverse(lp, e);

  while (e) {
    tw_event *n = e->cause_next;
    e->cause_next = NULL;

    event_cancel(e);
    e = n;
  }

  event->caused_by_me = NULL;
}
