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

inline void free_output_buffer(tw_out *buffer) {
  buffer->next = PE_VALUE(output);
  PE_VALUE(output) = buffer;
}

// Fill and send a regular message, possibly using short-circuiting
void charm_event_send(unsiged dest_peid, tw_event * e) {
  LP *send_pe = (LP*)(e->send_pe);
  LP *dest_pe;

  // Fill the fields of the charm message to prepare it for sending
  e->eventMsg->event_id = e->event_id = ((LP*)(e->send_pe))->uniqID++;
  e->eventMsg->ts = e->ts;
  e->eventMsg->dest_lp = e->dest_lp;
  e->eventMsg->send_pe = e->send_pe = ((LP*)(e->send_pe))->thisIndex;

  // Check for possible short-circuiting and send the message
  if (dest_peid == send_pe->thisIndex) {
    send_pe->recv_event(e->eventMsg);
  } else if ((dest_pe = lps(dest_peid).ckLocal()) != NULL &&
      e->ts > dest_pe->current_time) {
    dest_pe->recv_event(e->eventMsg);
  } else {
    lps(dest_peid).recv_event(e->eventMsg);
  }
}

// Fill and send an anti-message to cancel e
void charm_anti_send(tw_event * e) {
  unsigned dest_peid = ((tw_lp*)e->src_lp)->type->chare_map(e->dest_lp);
  RemoteEvent * eventMsg = new (0) RemoteEvent;
  eventMsg->isAnti = true;
  eventMsg->event_id = e->event_id;
  eventMsg->ts = e->ts;
  eventMsg->dest_lp = e->dest_lp;
  eventMsg->send_pe = e->send_pe;

  // TODO: Also include short-circuit logic here
  lps(dest_peid).recv_event(eventMsg);
}

#include "event.def.h"
