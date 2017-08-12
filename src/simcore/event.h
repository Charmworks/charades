/**
 * \file event.h
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "event.decl.h"

#include "typedefs.h"

class LPBase;

struct RemoteEvent : public CMessage_RemoteEvent {
  public:
    RemoteEvent() {
      clear();
    }
    void clear() {
      ts = 0;
      event_id = 0;
      src_lp = 0;
      dest_lp = 0;
    }

    char *userData;

    // Used for the async completion detection algorithm
    unsigned phase;

    // These three fields make up the unique key for identifying events
    Time ts;
    uint64_t event_id;
    uint64_t src_lp;

    // The global id of the destination lp
    uint64_t dest_lp;

    virtual void pup(PUP::er& p);
};

enum tw_event_owner {
  TW_event_null = 0,  /**< Event in unknown location */
  TW_chare_q = 1,     /**< In the chare's pending queue */
  TW_rollback_q = 2,  /**< In the chare's rollback queue */
  TW_sent = 3,        /**< Event sent to someone else */
};

struct tw_event_state {
  unsigned char owner;    /**< Which queue I am in; see tw_event_owner */
  unsigned char cancel_q; /**< Actively on a dest_lp->pe's cancel_q */
  unsigned char remote;   /**< Indicates union addr is in 'remote' storage */
  unsigned char avl_tree; /**< Indicates that the event is in the AVL tree */
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
static inline void reset_bitfields(Event* revent);

class Event {
public:
  Event() {
    clear();
  }
  void clear() {
    reset_bitfields(this);

    ts = 0;
    event_id = dest_lp = src_lp = 0;

    owner = NULL;
    eventMsg = NULL;

    state.owner = state.remote = state.cancel_q = state.avl_tree;

    next = prev = caused_by_me = cause_next = cancel_next = NULL;

    index = 0;

    pending_count = processed_count = sent_count = 0;
    pending_indices = processed_indices = NULL;
  }

  char* userData() const { return eventMsg->userData; }

  // Basic event info, used as a key for unique event identification
  Time ts;
  uint64_t event_id; // Unique increasing id per LP chare
  uint64_t src_lp;   // Pointer on sender side, not needed at destination
  uint64_t dest_lp;  // GID on sender side, pointer at destination

  LPBase* owner;
  RemoteEvent * eventMsg;

  // Fields used to enable time warp mechanism to do rollbacks
  tw_bf cv;             // Bitfield keeps track of execution path
  tw_event_state state; // State keeps track of who owns the event

  // Pointers used in data structures storing Events
  Event* prev;          // Prev in processed queue
  Event* next;          // Next in processed queue
  Event* caused_by_me;  // Start of event list caused by this event
  Event* cause_next;    // Next in parent's caused_by_me chain
  Event* cancel_next;   // next in cancel list

  // Index of the event in the pending heap. Also the order of event pupping
  size_t    index;

  // Fields for rebuilding causality lists after migration
  unsigned  pending_count;
  unsigned  processed_count;
  unsigned  sent_count;
  unsigned* pending_indices;
  unsigned* processed_indices;
};

// Set all bits in an events bitfield to 0
static inline void reset_bitfields(Event* revent) {
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
static inline void link_causality(Event* nev, Event* cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}

// Public API for models to use for managing events
/**
 * Allocates an event for the model to send.
 * \param dest_gid the global ID of the destination LP
 * \param offset_ts the offset in virtual time from the senders current time
 * \param sender a pointer to the sending LP
 * \returns a pointer to a new event
 */
Event* tw_event_new(uint64_t dest_gid, Time offset_ts, LPBase* sender);
/**
 * Send a previously allocated event.
 * \param event the event to send
 */
void tw_event_send(Event* event);
void tw_event_free(Event* e, bool commit);
void tw_event_rollback(Event* event);

// TODO: After unifying events this won't be needed
// API for Charm++ specific event usage, to be used by original ROSS code
Event* charm_allocate_event(int needMsg = 1);
void charm_free_event(Event* e);
void charm_event_cancel(Event* e);
int charm_event_send(unsigned, Event* e);
void charm_anti_send(unsigned, Event* e);

// Methods for pupping differnet types of events
void pup_pending_event(PUP::er& p, Event* e);
void pup_processed_event(PUP::er& p, Event* e);
void pup_sent_event(PUP::er& p, Event* e);
void pup_causality(PUP::er& p, Event* e);

#endif
