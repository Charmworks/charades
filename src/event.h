#ifndef _EVENT_H
#define _EVENT_H

#include "typedefs.h"

#include "ross.decl.h"

// TODO - not all are needed
enum tw_event_owner
{
  TW_pe_event_q = 1,	  /**< @brief In a tw_pe.event_q list */
  TW_pe_pq = 2,	       	  /**< @brief In a tw_pe.pq */
  TW_kp_pevent_q = 3,     /**< @brief In a tw_kp.pevent_q */
  TW_pe_anti_msg = 4,     /**< @brief Anti-message */
  TW_net_outq = 5,        /**< @brief Pending network transmission */
  TW_net_asend = 6,       /**< @brief Network transmission in progress */
  TW_net_acancel = 7,     /**< @brief Network transmission in progress */
  TW_pe_sevent_q = 8,     /**< @brief In tw_pe.sevent_q */
  TW_pe_free_q = 9        /**< @brief In tw_pe.free_q */
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
    tw_kp *owner;
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
  tw_peid send_pe;
  Time ts;
};


class Event {
  public:
  Event() {
    userData = NULL;
    eventMsg = NULL;
  }

  Event *next, *prev; //for processed queue
  size_t heap_index; //for avl trees
  Event *caused_by_me; //Start of event list caused by this event
  Event *cause_next; //Next in parent's caused_by_me chain

  EventID event_id;
  struct {
    unsigned char owner; 		/**< @brief Owner of the next/prev pointers; see tw_event_owner */
    unsigned char cancel_q;  	        /**< @brief Actively on a dest_lp->pe's cancel_q */
    unsigned char cancel_asend;
    unsigned char remote; 		/**< @brief Indicates union addr is in 'remote' storage */
  } state;

  tw_bf cv;
  tw_lpid dest_lp, src_lp;
  Time ts;
  tw_peid send_pe;
  RemoteEvent * eventMsg;
  tw_out *out_msgs;
  char *userData;
};

void tw_event_send(tw_event * event);
tw_event* tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender);

#endif
