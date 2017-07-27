#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "scheduler.decl.h"

#include "gvtmanager.h" // Temporary for produce/consume
#include "pe_queue.h"
#include "typedefs.h"

extern CkGroupID scheduler_id;

class Globals;
class LPChare;
class LPToken;
class RemoteEvent;
class Statistics;
class Trigger;
class tw_rng;

using std::string;

#if CMK_USING_XLC
typedef std::auto_ptr<Trigger> TriggerPtr;
#else
typedef std::unique_ptr<Trigger> TriggerPtr;
#endif

/**
 * Base class defining basic scheduler functionality. This requires keeping a
 * heap of LPs ordered by next event, methods for updating this heap, the
 * ability to execute events on these LPs, and basic statistics and global
 * variable management.
 */
class Scheduler : public CBase_Scheduler {
  protected:
    string scheduler_name;  /**< Name of scheduler for print outs */
    bool   running;         /**< True if there are active execute() messages */
    double start_time;      /**< Start wall time for the simulation */
    double end_time;        /**< End wall time for the simulation */

    PEQueue next_lps; /**< queue storing LPTokens ordered by next event ts */
    tw_rng * rng;     /**< ROSS rng stream */

  public:
    // TODO: Globals may not be needed once event handling is moved to scheduler
    Globals* globals;             /**< Global variables per PE */
    Statistics* stats;            /**< Statistics for current stat interval */
    Statistics* cumulative_stats; /**< Cumulative stats over all intervals */

    Scheduler();

    /** Entry method triggered by QD when all group chares are created */
    virtual void groups_created();

    /** Methods for starting and stopping the entire simulation */
    void start_simulation();  /**< Sets initial state and calls execute() */
    void end_simulation();    /**< Starts stats reduction */
    void finalize(CkReductionMsg *m); /**< Receives stats reduction and exits */

    /** Method for printing out basic info about the current progress */
    void print_progress(Time ts);

    /** Calls execute_me() on the next LP in the queue */
    bool schedule_next_lp();

    /** Get the minimum time of any event on this PE */
    virtual Time get_min_time() const;

    /** Entry method for executing a scheduler iteration */
    virtual void execute();

    /** Update the queue of LPs that are local to this PE */
    void register_lp(LPToken* next_token, Time next_ts) {
      next_lps.insert(next_token, next_ts);
    }
    void unregister_lp(LPToken* next_token) {
      next_lps.remove(next_token);
    }
    void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }

    // TODO: Move to distributed/optimistic
    virtual void consume(RemoteEvent* e) {}
    virtual void produce(RemoteEvent* e) {}
    virtual void update_min_cancel(Time ts) {}
};

/**
 * Concrete type for sequential schedulers. Only can be run for one PE, executes
 * the entire simulation in a single iteration, and doesn't need to worry about
 * GVT, migration, rollback, or cancellation.
 */
class SequentialScheduler : public CBase_SequentialScheduler {
  public:
    SequentialScheduler();
    void execute();
};

/**
 * Concrete base class for distributed schedulers. Unlike a sequential scheduler
 * distributed schedulers need to worry about GVTs, and load balancing. Because
 * of this, execution is broken up into iterations. Each iteration is a call to
 * execute() and we need to ensure that no more than one execute() message is
 * ever in flight.
 */
class DistributedScheduler : public CBase_DistributedScheduler {
  protected:
    GVTManager* gvt_manager;  /**< Direct pointer to our local GVT Manager */

    TriggerPtr gvt_trigger;   /**< Determines when to compute GVT */
    TriggerPtr lb_trigger;    /**< Determines when to do LB */
    TriggerPtr print_trigger; /**< Determines when to print progress */
#if CMK_TRACE_ENABLED
    TriggerPtr stat_trigger;  /**< Determines when to log stats */
#endif

  public:
    DistributedScheduler();

    /** Called after all group chares are created */
    void groups_created();

    /** Local methods for iteration control flow */
    void iteration_done();
    void next_iteration();

    /** Methods for load balancing synchronization */
    void start_balancing();
    void balancing_complete();

    /** Methods called by the GVT Manager signifying the scheduler may resume */
    virtual void gvt_resume();        /**< Called when LPs can unblock */
    virtual void gvt_done(Time gvt);  /**< Called when GVT is complete */

    /** Methods informing the GVT Manager about incoming/outgoing events */
    void consume(RemoteEvent* e) {
      gvt_manager->consume(e);
    }
    void produce(RemoteEvent* e) {
      gvt_manager->produce(e);
    }
};

#endif
