#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "scheduler.decl.h"

#include "typedefs.h"
#include "pe_queue.h"
#include "lp.h" // Temporary for LPToken
#include "charm_functions.h" // temporary for DEBUG_PE
#include "gvtmanager.h" // temporary for produce/consume

extern CkGroupID scheduler_id;

class RemoteEvent;
class LP;
class LPToken;
class PEManager;
class Globals;
class Statistics;
class Trigger;

using std::vector;

class Scheduler : public CBase_Scheduler {
  Scheduler_SDAG_CODE
  protected:
    /** LP queue variables */
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */

    /** Timer variables TODO: Just move to stats. */
    double start_time;  /**< Start wall time for the simulation */
    double end_time;    /**< End wall time for the simulation */

    /** Misc variables */
    tw_rng * rng; /**< ROSS rng stream */
    bool running; /**< execute() messages are current in-flight or running */

    std::string scheduler_name;

  public:
    // TODO: Globals may not be needed once event handling is moved to scheduler
    Globals* globals;             /**< Global variables per PE */
    Statistics* stats;            /**< Statistics for current stat interval */
    Statistics* cumulative_stats; /**< Cumulative stats over all intervals */

    Scheduler();

    virtual void groups_created();

    void finalize(CkReductionMsg *m);
    void start_simulation();
    void end_simulation();

    /** Entry method for executing a scheduler iteration */
    virtual void execute();

    /** Calls execute_me() on the next LP in the queue */
    virtual bool schedule_next_lp();

    /** Local accessor for getting the minimum time for GVT computation. It may
     *  vary based on the type of scheduler and potentially the type of GVT */
    virtual Time get_min_time() const;

    /** TODO: Most of the following should be moved to PE manager? */
    virtual void register_lp(LPToken* next_token, Time next_ts) {
      next_lps.insert(next_token, next_ts);
    }
    virtual void unregister_lp(LPToken* next_token) {
      next_lps.remove(next_token);
    }
    virtual void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }

    virtual void consume(RemoteEvent* e) {}
    virtual void produce(RemoteEvent* e) {}
    virtual void add_to_cancel_q(LP* lp) {}
    virtual void update_min_cancel(Time ts) {}
};

class SequentialScheduler : public CBase_SequentialScheduler {
  public:
    SequentialScheduler();
    void execute();
};

class DistributedScheduler : public CBase_DistributedScheduler {
  protected:
    GVTManager* gvt_manager;  /**< Direct pointer to our local GVT Manager */

    std::unique_ptr<Trigger> gvt_trigger; /**< Determines when to compute GVT */
    std::unique_ptr<Trigger> lb_trigger;  /**< Determines when to do LB */
#if CMK_TRACE_ENABLED
    std::unique_ptr<Trigger> stat_trigger;  /**< Determines when to log stats */
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

    /** Local method which allows the GVT manager to signify LP execution can
     *  continue without messing up the GVT. The specific scheduler subclasses
     *  will decide whether or not anything can be done at this point. */
    virtual void gvt_resume();
    /** Local method which allows the GVT manager to signify that the GVT is
     *  comleted with the passed in GVT being the result. */
    virtual void gvt_done(Time gvt);

    void consume(RemoteEvent* e) {
      gvt_manager->consume(e);
    }
    void produce(RemoteEvent* e) {
      gvt_manager->produce(e);
    }
};

#endif
