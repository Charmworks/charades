#ifndef _EVENT_H
#define _EVENT_H

#include "typedefs.h"

#include "event.decl.h"

enum tw_event_owner
{
  TW_event_inf = 0,      /**< End of line event */
  TW_event_null = 1,      /**< event in unused queue */
  TW_chare_q = 2,     /**<  In the chare's to be executed event queue */
  TW_rollback_q = 3,     /**< In the chare's rollback queue */
  TW_anti_msg = 4,     /**< Anti-message */
  TW_sent = 5, /**< Event sent to someone else */
  TW_pe_pq = 6
};

/**
 * tw_bf
 * @brief Reverse Computation Bitfield
 *
 * Some applications find it handy to have this bitfield when doing
 * reverse computation.  So we follow GTW tradition and provide it.
 */
struct tw_bf
{
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

typedef struct tw_out {
    struct tw_out *next;
// TODO: No kps in new ROSS
//    tw_kp *owner;
    /** The actual message content */
    char message[256 - 2*sizeof(void *)];
} tw_out;


// TODO: This needs some work, especially since we don't know how we are dealing
// with globals such as type yet. It would also be nice if we could decouple an
// event struct from the Charm++ infrastructure.
struct RemoteEvent : public CMessage_RemoteEvent {
  public:
  char *userData;
  EventID event_id;
  Time ts;
  tw_lpid dest_lp;
// TODO: Define what the peid type is
//  tw_peid send_pe;
  bool isAnti;

  RemoteEvent() : isAnti(false) { }
};


class Event {
  public:
  Event() {
    userData = NULL;
    eventMsg = NULL;
    caused_by_me = NULL;
    cause_next = NULL;
    cancel_next = NULL;
  }

  Event *next, *prev; //for processed queue
  size_t heap_index; //for avl trees
  Event *caused_by_me; //Start of event list caused by this event
  Event *cause_next; //Next in parent's caused_by_me chain
  Event *cancel_next; //next in cancel list

  EventID event_id;
  struct {
    unsigned char owner; 		/**< which queue am I in; see tw_event_owner */
    unsigned char cancel_q;  	        /**< @brief Actively on a dest_lp->pe's cancel_q */
    unsigned char remote; 		/**< @brief Indicates union addr is in 'remote' storage */
  } state;

  tw_bf cv;
  tw_lpid dest_lp, src_lp;
  Time ts;
// TODO: Define what the peid type is
//  tw_peid send_pe;
  RemoteEvent * eventMsg;
  tw_out *out_msgs;
  char *userData;
};

// Exposed event API
// TODO: Finalize/clean this up
void tw_event_send(tw_event * event);
tw_event* tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender);
tw_event * allocateEvent(int needMsg = 1);
void tw_event_rollback(tw_event * event);
void tw_event_free(tw_pe *pe, tw_event *e);
void event_cancel(tw_event * e);

#endif
