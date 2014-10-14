#include "ross_event.h"
#include "ross_util.h"

#include "lp_struct.h"
#include "charm_functions.h"
#include "charm_api.h"
#include "globals.h"
#include "ross_api.h"

#include <assert.h>

// Inline helper functions not exposed to the user
static inline void link_causality (tw_event *nev, tw_event *cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}

static inline void free_output_buffer(tw_out *buffer) {
  buffer->next = PE_VALUE(output);
  PE_VALUE(output) = buffer;
}

// TODO: See if this is supposed to be part of the public API or not
// TODO: Is this ever called with print_message == 1!?
static inline void tw_free_output_messages(tw_event *e, int print_message) {
  while (e->out_msgs) {
    tw_out *temp = e->out_msgs;
    if (print_message) {
      printf("%s", temp->message);
    }
    e->out_msgs = temp->next;
    // Put it back
    free_output_buffer(temp);
  }
  e->out_msgs = NULL;
}

// Public functions exposed to the user for allocating, sending, freeing, and
// rolling back events.
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
    e = charm_allocate_event();
  }

  e->dest_lp = dest_gid;
  e->src_lp = (tw_lpid)sender;
  e->ts = recv_ts;
  e->send_pe = (tw_peid)(sender->owner);

  // TODO: Why is this needed?
  tw_free_output_messages(e, 0);

  return e;
}

void tw_event_free(tw_pe *pe, tw_event *e) {
  tw_free_output_messages(e, 0);
  DEBUG2("Free event %d %d %lf\n", e->send_pe, e->event_id, e->ts);
  charm_free_event(e);
}

void tw_event_send(tw_event * e) {
  tw_lp* src_lp = (tw_lp*)e->src_lp;
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

  link_causality(e, current_event(src_lp));

  // Call LP remote mapping function to get dest_pe
  dest_peid = src_lp->type->chare_map(e->dest_lp);

  // The charm backend will fill in the remote event and send it
  charm_event_send(dest_peid, e); 

  // Deallocate the event or unlink the event message from the event
  // depending on the synchronization protocol
  if(PE_VALUE(g_tw_synchronization_protocol) != OPTIMISTIC) {
    // TODO: Have logic for handling eventMsg pointers encapsulated in send/free
    // methods rather than having ROSS needing to know how to deal with it.
    e->eventMsg = NULL;
    charm_free_event(e);
  } else {
    e->state.owner = TW_sent;
    e->eventMsg = NULL;
  }
}

void tw_event_rollback(tw_event * event) {
  tw_event  *e = event->caused_by_me;
  tw_lp     *dest_lp = (tw_lp*)event->dest_lp;

  // TODO: Why are output messages freed here?
  tw_free_output_messages(event, 0);

  set_current_event(dest_lp, event);
  dest_lp->type->reverse(dest_lp->state, &event->cv, tw_event_data(event), dest_lp);
  (PE_VALUE(netEvents))--;

  while (e) {
    tw_event *n = e->cause_next;
    e->cause_next = NULL;

    charm_event_cancel(e);
    e = n;
  }

  event->caused_by_me = NULL;
}

// TODO: Is this supposed to be publicly exposed?
tw_out* allocate_output_buffer() {
  tw_out* free_buf = NULL;
  if(PE_VALUE(output)) {
    free_buf = PE_VALUE(output);
    PE_VALUE(output) = free_buf->next;
    free_buf->next = NULL;
  }
  return free_buf;
}
