#ifndef ROSS_EVENT_H_
#define ROSS_EVENT_H_

#include "typedefs.h"

#include "string.h"

enum tw_event_owner {
  TW_event_null = 0,  /**< Event in unknown location */
  TW_chare_q = 1,     /**< In the chare's pending queue */
  TW_rollback_q = 2,  /**< In the chare's rollback queue */
  TW_sent = 3,        /**< Event sent to someone else */
};

/**
 * tw_bf
 * @brief Reverse Computation Bitfield
 *
 * Some applications find it handy to have this bitfield when doing
 * reverse computation.  So we follow GTW tradition and provide it.
 */
struct tw_bf {
  unsigned int    c0:1;
  unsigned int    c1:1;
  unsigned int    c2:1;
  unsigned int    c3:1;
  unsigned int    c4:1;
  unsigned int    c5:1;
  unsigned int    c6:1;
  unsigned int    c7:1;
  unsigned int    c8:1;
  unsigned int    c9:1;
  unsigned int    c10:1;
  unsigned int    c11:1;
  unsigned int    c12:1;
  unsigned int    c13:1;
  unsigned int    c14:1;
  unsigned int    c15:1;
  unsigned int    c16:1;
  unsigned int    c17:1;
  unsigned int    c18:1;
  unsigned int    c19:1;
  unsigned int    c20:1;
  unsigned int    c21:1;
  unsigned int    c22:1;
  unsigned int    c23:1;
  unsigned int    c24:1;
  unsigned int    c25:1;
  unsigned int    c26:1;
  unsigned int    c27:1;
  unsigned int    c28:1;
  unsigned int    c29:1;
  unsigned int    c30:1;
  unsigned int    c31:1;
};

struct tw_event_state {
  unsigned char owner;    /**< Which queue I am in; see tw_event_owner */
  unsigned char cancel_q; /**< Actively on a dest_lp->pe's cancel_q */
  unsigned char remote;   /**< Indicates union addr is in 'remote' storage */
};

struct tw_out {
    struct tw_out *next;
    char message[256 - 2*sizeof(void *)];
};

class RemoteEvent;
class Event {
  public:
  Event() {
    userData = NULL;
    eventMsg = NULL;
    caused_by_me = NULL;
    cause_next = NULL;
    cancel_next = NULL;
    out_msgs = NULL;
    state.remote = 0;
    state.cancel_q = 0;
  }
  // Basic event info
  EventID event_id;
  Time ts;
  tw_bf cv;

  // The event state says which queues the event is in and whether it is remote
  tw_event_state state;

  // Fields for sender/receiver info. Can be cast as ids or ptrs.
  tw_lpid dest_lp, src_lp;
  tw_peid send_pe;

  // Fields storing msg data
  RemoteEvent * eventMsg;
  char *userData;

  // Field storing output messages tied to this event
  tw_out *out_msgs;

  // Fields used in data structures storing Events
  size_t heap_index;    // for avl trees
  Event* up;            // for splay trees
  Event *next, *prev;   // for splay trees and processed queue
  Event *caused_by_me;  // Start of event list caused by this event
  Event *cause_next;    // Next in parent's caused_by_me chain
  Event *cancel_next;   // next in cancel list

  // Fields for rebuilding causality lists after migration
  unsigned seq_num;     // Gives the order in which the event was pupped
  unsigned* pending_indices;
  unsigned pending_count;
  unsigned* processed_indices;
  unsigned processed_count;
  unsigned sent_count;
};

// Publicly exposed functions
tw_event* tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender);
void tw_event_free(tw_pe *pe, tw_event *e);
void tw_event_send(tw_event * event);
void tw_event_rollback(tw_event * event);

// TODO: Should this be here
tw_out* allocate_output_buffer();
// TODO: Should this be here?
static inline void reset_bitfields(tw_event *revent) {
  if (sizeof(revent->cv) == sizeof(uint32_t)){
    *(uint32_t*)&revent->cv = 0;
  }
  else if (sizeof(revent->cv) == sizeof(uint64_t)){
    *(uint64_t*)&revent->cv = 0;
  }
  else{
    memset(&revent->cv, 0, sizeof(revent->cv));
  }
}

// This function is also used during unpacking after migration, which is why
// it is included in the header instead of the cpp file.
static inline void link_causality (tw_event *nev, tw_event *cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}


#endif
