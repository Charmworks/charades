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
      event_id = 0;
      ts = 0.0;
      send_pe = 0;
      dest_lp = 0;
    }

    char *userData;

    // Used for the async completion detection algorithm
    unsigned phase;

    // These three fields make up the unique key for identifying events
    EventID event_id;
    Time ts;
    tw_peid send_pe;

    // The global id of the destination lp
    tw_lpid dest_lp;

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

    event_id = 0;
    ts = 0.0;

    state.owner = state.remote = state.cancel_q = state.avl_tree;

    dest_lp = src_lp = 0;
    send_pe = 0;

    eventMsg = NULL;
    userData = NULL;

    up = next = prev = NULL;
    caused_by_me = cause_next = cancel_next = NULL;

    index = 0;

    pending_count = processed_count = sent_count = 0;
    pending_indices = processed_indices = NULL;
  }

  // Basic event info, used as a key for unique event identification
  EventID event_id; // Unique increasing id per LP chare.
  tw_peid send_pe;  // Cast as a pointer to LP chare before sent.
  Time ts;

  // Fields used to enable time warp mechanism to do rollbacks
  tw_bf cv;             // Bitfield keeps track of execution path
  tw_event_state state; // State keeps track of who owns the event

  // Source and dest LP may either be pointers or gids
  tw_lpid dest_lp;  // GID on sender side, pointer at destination
  tw_lpid src_lp;   // Pointer on sender side, not needed at destination

  // Fields storing msg data
  RemoteEvent * eventMsg;
  char *userData;

  // Pointers used in data structures storing Events
  Event* up;            // Parent in splay trees
  Event* prev;          // Prev in processed queue, or left child in splay tree
  Event* next;          // Next in processed queue, or right child in splay tree
  Event* caused_by_me;  // Start of event list caused by this event
  Event* cause_next;    // Next in parent's caused_by_me chain
  Event* cancel_next;   // next in cancel list

  // Index of the event in the pending heap. Also the order of event pupping.
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
static inline void link_causality (tw_event *nev, tw_event *cev) {
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
tw_event* tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, LPBase* sender);
/**
 * Send a previously allocated event.
 * \param event the event to send
 */
void tw_event_send(tw_event * event);
void tw_event_free(tw_event *e, bool commit);
void tw_event_rollback(tw_event * event);

/**
 * Macro for getting the model specific data from the event.
 * \param e the event to extract the data from
 * \returns a pointer to the user data
 */
#define tw_event_data(e) (e->userData)

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
