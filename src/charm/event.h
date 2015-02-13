#ifndef EVENT_H_
#define EVENT_H_

#include "event.decl.h"
#include "typedefs.h"

struct RemoteEvent : public CMessage_RemoteEvent {
  public:
    char *userData;

    // These three fields make up the unique key for identifying events
    EventID event_id;
    Time ts;
    tw_peid send_pe;

    // The global id of the destination lp
    tw_lpid dest_lp;
};

class Event;
inline void operator|(PUP::er& p, Event* e) {
  // Basic pupping
  p | e->seq_num;  
  p | e->state;
  p | e->cv;
  p | e->ts;

  // TODO: Things that can be pointers or ints need to be handled correctly
  // TODO: These may all have to be converted to ints before pupping
  p | e->dest_lp;
  p | e->src_lp;
  p | e->send_pe;

  // Pupping the remote message data
  p | e->eventMsg;
  if (p.isUnpacking() && e->eventMsg) {
    e->userData = e->eventMsg->userData;
  }

  // TODO: Figure out how to pup out_msgs
  //p | out_msgs;

  // Pupping causality links
  Event* tmp;
  if (p.isPacking()) {
    e->pending_count = 0;
    e->processed_count = 0;
    tmp = e->caused_by_me;
    while (tmp) {
      if (tmp->state.owner = TW_chare_q) {
        e->pending_count++;
      } else if (tmp->state.owner = TW_rollback_q) {
        e->processed_count++;
      } else {
        // TODO: What to do with sent events?
      }
      tmp = tmp->cause_next;
    }

    e->pending_indices = new unsigned[e->pending_count];
    e->processed_indices = new unsigned[e->processed_count];

    // NOTE: This only works because causal events will either be in the
    // pending queue, or in the processed queue, but pupped before this one.
    // This also implies that the pending queue must be pupped before the
    // processed queue. If an event gets pupped before an event it caused,
    // this whole process will fall apart.
    unsigned pending_idx = 0;
    unsigned processed_idx = 0;
    tmp = e->caused_by_me;
    while (tmp) {
      if (tmp->state.owner = TW_chare_q) {
        e->pending_indices[pending_idx++] = tmp->seq_num;
      } else if (tmp->state.owner = TW_rollback_q) {
        e->processed_indices[processed_idx++] = tmp->seq_num;
      }
      tmp = tmp->cause_next;
    }
  }
  p | e->pending_count;
  p | e->processed_count;

  if (p.isUnpacking()) {
    e->pending_indices = new unsigned[e->pending_count];
    e->processed_indices = new unsigned[e->processed_count];
  }
  PUParray(p, e->pending_indices, e->pending_count);
  PUParray(p, e->processed_indices, e->processed_count);
  // TODO: What to do with sent events
}

#endif
