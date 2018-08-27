/**
 * \file event.h
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "event.decl.h"

#include "globals.h" // Included for g_num_msg_types (TODO should be to move this)
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

    Time ts;
    uint64_t event_id;
    uint64_t src_lp;
    uint64_t dest_lp;

    uint8_t phase;    /**< Used for async phased GVT calculation */
    uint8_t type_id;  /**< Used for double-dispatch of event data */
    uint8_t type_size;
    char* data;       /**< Points to memory for user event data */

    virtual void pup(PUP::er& p) {
      p | ts;
      p | event_id;
      p | src_lp;
      p | dest_lp;
      p | phase;
      p | type_id;
      p | type_size;
      p(data, type_size);
    }
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
PUPbytes(tw_event_state);

/**
 * tw_bf
 * @brief Reverse Computation Bitfield
 *
 * Some applications find it handy to have this bitfield when doing
 * reverse computation.  So we follow GTW tradition and provide it.
 */
struct tw_bf {
  unsigned int c0:1;
  unsigned int c1:1;
  unsigned int c2:1;
  unsigned int c3:1;
  unsigned int c4:1;
  unsigned int c5:1;
  unsigned int c6:1;
  unsigned int c7:1;
  unsigned int c8:1;
  unsigned int c9:1;
  unsigned int c10:1;
  unsigned int c11:1;
  unsigned int c12:1;
  unsigned int c13:1;
  unsigned int c14:1;
  unsigned int c15:1;
  unsigned int c16:1;
  unsigned int c17:1;
  unsigned int c18:1;
  unsigned int c19:1;
  unsigned int c20:1;
  unsigned int c21:1;
  unsigned int c22:1;
  unsigned int c23:1;
  unsigned int c24:1;
  unsigned int c25:1;
  unsigned int c26:1;
  unsigned int c27:1;
  unsigned int c28:1;
  unsigned int c29:1;
  unsigned int c30:1;
  unsigned int c31:1;

  void clear() {
    memset(this, 0, sizeof(tw_bf));
  }
};
PUPbytes(tw_bf);

void pup_causality(PUP::er& p, Event* e);

class Event {
public:
  // Basic event info, used as a key for unique event identification
  Time ts;
  uint64_t event_id;
  uint64_t src_lp;
  uint64_t dest_lp;

  LPBase* owner;
  RemoteEvent* msg;

  // Variables for message data types
  uint8_t type_id;
  uint8_t type_size;

  // Fields used to enable time warp mechanism to do rollbacks
  tw_bf cv;             // Bitfield keeps track of execution path
  tw_event_state state; // State keeps track of who owns the event

  // Pointers used in data structures storing Events
  Event* prev;          // Prev in processed queue
  Event* next;          // Next in processed queue
  Event* caused_by_me;  // Start of event list caused by this event
  Event* cause_next;    // Next in parent's caused_by_me chain
  Event* cancel_next;   // next in cancel list

  // Index of the event in the pending heap and the order of event pupping
  uint8_t index;

  // Fields for rebuilding causality lists after migration
  unsigned  pending_count;
  unsigned  processed_count;
  unsigned  sent_count;
  unsigned* pending_indices;
  unsigned* processed_indices;

  Event() {
    clear();
  }
  void clear() {
    ts = 0;
    event_id = dest_lp = src_lp = 0;

    owner = NULL;
    msg = NULL;

    cv.clear();
    state.owner = state.remote = state.cancel_q = state.avl_tree = 0;

    next = prev = caused_by_me = cause_next = cancel_next = NULL;

    type_id = type_size = index = 0;

    pending_count = processed_count = sent_count = 0;
    pending_indices = processed_indices = NULL;
  }
  void pup(PUP::er& p) {
    p | state;

    if (state.owner == TW_chare_q) {
      p | index;
      p | type_size;
      if (p.isUnpacking()) {
        msg = new (type_size) RemoteEvent();
      }
      msg->pup(p);
      set_msg(msg);
    } else if (state.owner == TW_rollback_q) {
      p | cv;
      p | index;
      p | type_size;
      if (p.isUnpacking()) {
        msg = new (type_size) RemoteEvent();
      }
      msg->pup(p);
      set_msg(msg);
      pup_causality(p,this);
    } else if (state.owner == TW_sent) {
      p | ts;
      p | event_id;
      p | src_lp;
      p | dest_lp;
    } else {
      CkAbort("Bad event state during pupping\n");
    }
  }

  void set_msg(RemoteEvent* m) {
    msg  = m;
    ts        = msg->ts;
    event_id  = msg->event_id;
    src_lp    = msg->src_lp;
    dest_lp   = msg->dest_lp;
    type_id   = msg->type_id;
    type_size = msg->type_size;
  }
  RemoteEvent* get_msg() const {
    return msg;
  }

  template<typename DataType>
  DataType* get_data() const {
    return reinterpret_cast<DataType*>(msg->data);
  }
};

class StandardEventComparator {
  public:
    bool operator()(const Event* e1, const Event* e2);
};

class CustomEventComparator {
  public:
    bool operator()(const Event* e1, const Event* e2);
};

// This function is also used during unpacking after migration, which is why
// it is included in the header instead of the cpp file.
static inline void link_causality(Event* nev, Event* cev) {
  nev->cause_next = cev->caused_by_me;
  cev->caused_by_me = nev;
}

// Public API for models to use for managing events
/**
 * Send a previously allocated event.
 * \param event the event to send
 */
Event* event_alloc(RemoteEvent* event, uint64_t dest_gid, Time offset, LPBase * sender);
Event* event_alloc();
void tw_event_send(Event* event);
void tw_event_free(Event* e, bool commit);
void tw_event_rollback(Event* event);

// TODO: After unifying events this won't be needed
// API for Charm++ specific event usage, to be used by original ROSS code
void charm_event_cancel(Event* e);
void charm_anti_send(unsigned, Event* e);

template <typename MsgType>
uint32_t get_msg_id() {
  static uint32_t msg_id = g_num_msg_types++;
  return msg_id;
}

template <typename MsgType>
void register_msg_type() {
  get_msg_id<MsgType>();
}

template <typename LPType>
class DispatcherBase {
public:
  virtual void forward(LPType* lp, Event* e) = 0;
  virtual void reverse(LPType* lp, Event* e) = 0;
  virtual void commit(LPType* lp, Event* e) = 0;
};

template <typename LPType, typename MsgType>
class Dispatcher : public DispatcherBase<LPType> {
public:
  void forward(LPType* lp, Event* e) {
    lp->forward(e->get_data<MsgType>(), &e->cv);
  }
  void reverse(LPType* lp, Event* e) {
    lp->reverse(e->get_data<MsgType>(), &e->cv);
  }
  void commit(LPType* lp, Event* e) {
    lp->commit(e->get_data<MsgType>(), &e->cv);
  }
};

template <typename MsgType, typename... Args>
Event* tw_event_new(uint64_t dest_gid, Time offset, LPBase* sender, Args&&... args) {
  RemoteEvent* msg = new (sizeof(MsgType)) RemoteEvent();
  msg->type_id = get_msg_id<MsgType>();
  msg->type_size = sizeof(MsgType);
  new (msg->data) MsgType(std::forward<Args>(args)...);
  return event_alloc(msg, dest_gid, offset, sender);
}

Event* tw_event_new(uint64_t dest_gid, Time offset, LPBase* sender, size_t size);

#endif
