#ifndef _LP_H
#define _LP_H

#include "lp.decl.h"

#include "typedefs.h"
#include "lp_struct.h"
#include "ross_clcg4.h"

#include "processed_queue.h"

#ifdef USE_SPLAY
#include "pending_splay.h"
#else
#include "pending_heap.h"
#endif

#include <vector>

class RemoteEvent;
class Scheduler;

void operator|(PUP::er&, tw_rng_stream*);
void operator|(PUP::er&, LPStruct&);

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
    friend class PE;
    friend class Scheduler;
};

typedef std::vector<LPStruct> LPList;

class LP : public CBase_LP {
  private:
    // LP Tokens for pending events and fossils
    LPToken next_token;
    LPToken oldest_token;

    // All lps managed by this chare
    LPList lp_structs;

    // Queues for storing events
#ifdef USE_SPLAY
    PendingSplay events;
#else
    PendingHeap events;
#endif
    ProcessedQueue processed_events;

    // Cancel queue management
    Event *cancel_q;    // Queue of events this LP needs to cancel
    Time min_cancel_q;  // Minimum time in this LPs cancel queue
    bool in_pe_queue;   // Whether or not this LP is in the PE cancel queue

    // A direct pointer to the PE where this LP chare resides
    Scheduler* pe;

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

#endif
