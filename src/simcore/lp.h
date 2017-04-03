#ifndef _LP_H
#define _LP_H

#include "lp.decl.h"

#include "pending_heap.h"
#include "processed_queue.h"
#include "typedefs.h"

#include <vector>

extern CProxy_LP lps;

class RemoteEvent;
class Scheduler;
struct tw_rng_stream;

using std::vector;

// TODO: Why do it like this? Why not have the tokens defined as nodes of the
// queue instead?
// Tokens owned by LP chares that are used by the PE queues that control
// scheduling and fossil collection. Each token has a direct pointer to its LP,
// the timestamp associated with the token, and the index of its location in the
// queue.
struct LPToken {
  private:
    LP* lp;
    Time ts;
    unsigned index;

  public:
    LPToken(LP* lp) : lp(lp) {}
    LPToken() {}

    friend class PEQueue;
    friend class Scheduler;
    friend class DistributedScheduler;
    friend class OptimisticScheduler;
};

// The LPType contains function pointers for handling/reversing events as well
// as maps on how to locate LP structs based on their global ids.
struct LPType {
  init_f init;
  event_f execute;
  revent_f reverse;
  final_f finalize;
  commit_f commit;
  size_t state_size;
};

// Right now, an LPStruct is an LPType, as well as its state.
struct LPStruct {
  LP* owner;
  unsigned gid;
  void* state;
  LPType* type;
  tw_rng_stream* rng;
};

class LP : public CBase_LP {
  private:
    // LP Tokens for pending events and fossils
    LPToken next_token;
    LPToken oldest_token;

    // All lps managed by this chare
    vector<LPStruct> lp_structs;

    // Queues for storing events
    PendingHeap events;
    ProcessedQueue processed_events;

    // Cancel queue management
    Event *cancel_q;    // Queue of events this LP needs to cancel
    Time min_cancel_q;  // Minimum time in this LPs cancel queue
    bool in_pe_queue;   // Whether or not this LP is in the PE cancel queue

    // A direct pointer to the PE where this LP chare resides
    Scheduler* scheduler;

    // Some control flow varies when we are in optimistic mode
    bool isOptimistic;
  public:
    // Used to give a unique EventID to every message sent
    EventID uniqID;

    // AvlTree storing all events associated with this LP. Essentially a hash
    // used for cancellation purposes.
    AvlTree all_events;

    // Current state of this LP chare. Accessed through a C-style API by ROSS.
    Time current_time;
    Event *current_event;

    LP();
    LP(CkMigrateMessage* m);

    // Methods used for migration
    void reconstruct_causality(Event*, Event**, Event**);
    void reconstruct_pending_event(Event*);
    void reconstruct_processed_event(Event*, Event**, Event**);
    virtual void pup(PUP::er &p);
    void load_balance();
    virtual void ResumeFromSync();

    // After initializing lps, we stop the charm scheduler and return control
    // to ROSS until we are ready to start the simulation.
    void init();
    void stop_scheduler();

    // Methods for receiving events and anti events.
    void recv_remote_event(RemoteEvent*);
    void recv_anti_event(RemoteEvent*);
    void recv_local_event(Event*);

    // Execute a single event from the pending queue. If optimistic, we also
    // push the event onto the processed queue.
    void* execute_me();

    // Rollback and fossil collection only occur in optimistic mode.
    // When detecting a rollback upon receiving an event, we rollback to that
    // timestamp. When cancelling and event that would cause a rollback, we
    // rollback to that event before deleting it.
    // Fossil collection is called by the PE after GVT computation.
    void rollback_me(Time);
    void rollback_me(Event*);
    void fossil_me(Time);

    // Event cancellation events (only in optimistic)
    // When cancelling an event, we either delete it from our pending event
    // queue, or put it in the cancel_q for later if it would cause a rollback.
    // The PE will periodically call process_cancel_q() on LPs.
    void cancel_event(Event*);
    void delete_pending(Event*);
    void add_to_cancel_q(Event*);
    void process_cancel_q();

    Time min_cancel_time() const {
      return min_cancel_q;
    }
};

// TODO: Shouldn't need this anymore
// API for ROSS code to interact with LPs.
void init_lps();
void set_current_event(tw_lp*, tw_event*);
tw_event* current_event(tw_lp*);
tw_stime tw_now(tw_lp*);

#endif
