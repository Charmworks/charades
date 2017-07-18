/**
 * \file lp.h
 * Declarations for classes/variables/methods related to LPs
 */
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

/**
 * A token representing a handle to an LP chare inside the scheduler queues.
 * \todo Shouldn't this just be defined as a node in the data structure?
 */
struct LPToken {
  private:
    LP* lp;         ///< Direct pointer to the LP chare this token represents
    Time ts;        ///< A timestamp associated with the LP to be used as a key
    unsigned index; ///< The index of this token within the queue/heap

  public:
    /** Default constructor \todo should this be disabled? */
    LPToken() {}

    /**
     * Constructor called from the LP chare that sets lp at initialization
     * \param lp a pointer to the LP chare this token represents
     */
    LPToken(LP* lp) : lp(lp) {}

    friend class PEQueue;
    friend class Scheduler;
    friend class DistributedScheduler;
    friend class OptimisticScheduler;
};

/**
 * Defines a set of handlers as an LP type used by model LPs.
 * \todo Converting LPStruct to use class inheritance would deprecate this
 */
struct LPType {
  init_f init;        ///< Initialization handler for this LP type
  event_f execute;    ///< Forward event handler for this LP type
  revent_f reverse;   ///< Reverse event handler for this LP type
  commit_f commit;    ///< Commit event handler for this LP type
  final_f finalize;   ///< Finalization handler for this LP type
  size_t state_size;  ///< The size of the model state required for this LP type
};

/**
 * Model defined LP structure which encapsulates a type and state.
 * \todo convert to a C++ class inheriting from a base type? How easy would it
 * be to maintain backwards compatibility in that case?
 */
struct LPStruct {
  LP* owner;          ///< Pointer to the LP chare this LPStruct belongs to
  unsigned gid;       ///< Global LP ID
  void* state;        ///< Pointer to model specific state (size set in LPType)
  LPType* type;       ///< Type of this LP
  tw_rng_stream* rng; ///< Array of RNGs for this LP
};

/**
 * A chare that encapsulates a set of LPStructs and their events.
 * Everything the contained LPs need for execution should be contained in here
 * to make migration work correctly. At the moment, this is true except for the
 * PE level AVL tree.
 *
 * \todo See if moving the AVL tree from the scheduler to here would be feasible
 * and not mess up performance.
 *
 * \todo Converting AVL tree to an STL type or an array based structure would
 * make migration way simpler.
 */
class LP : public CBase_LP {
  private:
    /**
     * LPToken whose key is the timestamp of the next event this chare will
     * execute. It is updated when events are received, executed, or rolled
     * back
     */
    LPToken next_token;

    /**
     * All future events received for any of our LPs. The next_token LPToken
     * gets its timestamp from the top of this heap.
     */
    PendingHeap events;

    /**
     * A flat queue of all events previously processed by our LPs that haven't
     * yet been committed or rolled back. The oldest_token LPToken gets its
     * timestamp from the back of this queue.
     */
    ProcessedQueue processed_events;

    /**
     * A list of all LPs owned by this chare.
     * Initialized in the constructor based on maps set by the model.
     */
    vector<LPStruct> lp_structs;

    /**
     * A direct pointer to the scheduler managing this LP chare. The scheduler
     * map be sequential, optimistic, or conservative.
     * \todo Should the LP class be specialized based on sync protocol as well?
     */
    Scheduler* scheduler;

    /**
     * Set to true if this LP is an optimistic LP
     * \todo Should the LP class be specialised based on sync protocol as well?
     */
    bool isOptimistic;

    /**
     * \name LB metrics
     * Variables used to define load for custom load balancing metrics
     *////@{
    int committed_events;   ///< Number of events committed since last LB
    int rolled_back_events; ///< Number of events rolled back since last LB
    Time committed_time;    ///< Timestamp of most recent committed event
    Time latest_time;       ///< Latest timestamp reached by this LP
    ///@}

    /**
     * \name Cancellation Queue Variables
     * These variables manage the events that need to be cancelled in this LP.
     * Cancellation management has been simplified since the scheduler now
     * calls cancellation methods on all of its LPs.
     *///@{
    std::list<Event*> cancel_queue; ///< Queue of events this LP needs to cancel
    Time min_cancel_q;              ///< Minimum time in this LPs cancel queue
    ///@}

  public:
    EventID uniqID; ///< Used to give a unique ID to every event sent from here

    /**
     * A pointer to the PE level AVL tree for hashing remote events.
     * \todo should this be move to the LP level?
     */
    AvlTree all_events;

    Time current_time;    ///< Time of most recently executed event
    Event *current_event; ///< Most recently executed event

    /** Default constructor */
    LP();
    /** Migration constructor */
    LP(CkMigrateMessage* m);

    /** Stops the Charm++ scheduler and returns control to main */
    void stop_scheduler();

    /**
     * \name Scheduler Calls
     * These are regular methods called by the scheduler at various points
     * throughout simulation execution.
     */
    /** Called at the start of a simulation to run LP init handlers */
    void init();
    /**
     * Called during execution to execute the next event owned by this LP
     * \returns false if no events are able to be executed
     */
    void* execute_me();
    /**
     * Called after GVT computation to commit old events
     * \param gvt the current Global Virtual Time
     */
    void fossil_me(Time gvt);
    ///@}

    /**
     * \name Migration methods
     * These methods are used to enable migration for things like LB and
     * potentially checkpoint/restart.
     * \todo Some changes to events should be made to simplify/unify pupping
     *////@{
    /** Correctly rebuild causality chains when unpacking */
    void reconstruct_causality(Event*, Event**, Event**);
    /** Correclty rebuild pending events when unpacking */
    void reconstruct_pending_event(Event*);
    /** Correctly rebuild processed events when unpacking */
    void reconstruct_processed_event(Event*, Event**, Event**);
    /** Pack/unpack this LP chare */
    virtual void pup(PUP::er &p);
    /** Tell this chare that we are going to do load balancing */
    void load_balance();
    /** Called by the runtime system to tell this chare we can resume */
    void ResumeFromSync();
    /** Called by the runtime system to get the load of this chare */
    void UserSetLBLoad();
    ///@}

    /**
     * \name Event Receive Methods
     * Methods for receiving different types of events. Remote and Anti events
     * come from other chares so they require entry methods.
     *////@{
    /**
     * Entry method for receiving a remote event from another chare
     * \param event an event sent from a different chare to an LP on this chare
     */
    void recv_remote_event(RemoteEvent* event);
    /**
     * Entry method for receiving and processing an anti event
     * \param event an anti event sent from another chare
     */
    void recv_anti_event(RemoteEvent* event);
    /**
     * Regular method for common processing on locally allocated events
     * \param e a locally allocated Event sent by an LP on our chare or created
     * and filled in recv_remote_event
     */
    void recv_local_event(Event* e);
    ///@}

    /**
     * \name Rollback/Cancellation Methods
     * The purpose of these methods is recovery from causality violations so
     * therefore they are only used in optimistic mode.
     *////@{
    /**
     * Do a primary rollback to a given virtual timestamp
     * \param ts the virtual timestamp to rollback to
     */
    void rollback_me(Time ts);
    /**
     * Do a secondary rollback to a particular event
     * \param event the event to rollback to
     */
    void rollback_me(Event* event);
    /**
     * Cancel an event that should have never been received
     * \param e the event to be cancelled
     */
    void cancel_event(Event* event);
    /**
     * Cancel an event from the pending queue by simply removing it
     * \param e the event to delete from the pending queue
     */
    void delete_pending(Event* event);
    /**
     * Add an event to the queue of events to be cancelled later
     * \param e the event to add to the cancel queue
     */
    void add_to_cancel_q(Event* e);
    /** Process and cancel each event in the cancel queue */
    void process_cancel_q();
    ///@}
};

/**
 * \name API for ROSS
 * Allows old ROSS code to interact with LPs.
 * \todo A lot of this should be removed
 *////@{
void init_lps();
void set_current_event(tw_lp*, tw_event*);
tw_event* current_event(tw_lp*);
tw_stime tw_now(tw_lp*);
///@}

#endif
