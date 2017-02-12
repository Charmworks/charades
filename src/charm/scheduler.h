#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "scheduler.decl.h"

#include "typedefs.h"
#include "pe_queue.h"
#include "lp.h" // Temporary for LPToken

extern CProxy_Scheduler scheduler_proxy;

class RemoteEvent;
class GVTManager;
class LP;
class LPToken;
class PEManager;

using std::vector;

class Scheduler : public CBase_Scheduler {
  protected:
    /** LP queue variables */
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */

    /** Local pointers to other PE-level objects */
    PEManager* pe_manager;
    GVTManager* gvt_manager;

  public:
    Scheduler();

    /** Entry method for executing a scheduler iteration */
    virtual void execute();

    /** Local method which allows the GVT manager to signify LP execution can
     *  continue without messing up the GVT. The specific scheduler subclasses
     *  will decide whether or not anything can be done at this point. */
    virtual void gvt_resume();
    /** Local method which allows the GVT manager to signify that the GVT is
     *  comleted with the passed in GVT being the result. */
    virtual void gvt_done(Time gvt);

    /** Called by the local PEManager after all groups have been initialized */
    void set_local_pointers(PEManager* pem, GVTManager* gvtm) {
      pe_manager = pem;
      gvt_manager = gvtm;
    }

    /** Calls execute_me() on the next LP in the queue */
    virtual bool schedule_next_lp();

    /** Local accessor for getting the minimum time for GVT computation. It may
     *  vary based on the type of scheduler and potentially the type of GVT */
    virtual Time get_min_time() const;


    /** TODO: Most of the following should be moved to PE manager? */
    void register_lp(LPToken* next_token, Time next_ts) {
      next_lps.insert(next_token, next_ts);
    }
    void unregister_lp(LPToken* next_token) {
      next_lps.remove(next_token);
    }
    void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }
    virtual void add_to_cancel_q(LP* lp) {}
    virtual void update_min_cancel(Time ts) {}
};

class SequentialScheduler : public CBase_SequentialScheduler {
  public:
    SequentialScheduler();
    void execute();
};

class ConservativeScheduler : public CBase_ConservativeScheduler {
  public:
    ConservativeScheduler();
    void execute();
};

class OptimisticScheduler : public CBase_OptimisticScheduler {
  private:
    int iter_cnt;         /**< iteration count since last gvt */
    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

  public:
    OptimisticScheduler();
    void execute();
    void gvt_done(Time gvt);
    void collect_fossils(Time gvt);       /**< collect fossils */
    void process_cancel_q();      /**< process the cancel_q */
    void add_to_cancel_q(LP*);    /**< add an LP to the cancel_q */
    void update_min_cancel(Time); /**< update min_cancel_time */
    Time get_min_time() const;    /**< Override base method to include cancel q*/

    // TODO: What does it mean to remove from cancel queue if it's actually in
    // the cancel queue?
    void unregister_lp(LPToken* next_token) {
      vector<LP*>::iterator it = cancel_q.begin();
      while (it != cancel_q.end()) {
        if (*it == next_token->lp) {
          cancel_q.erase(it);
          break;
        }
        it++;
      }
      Scheduler::unregister_lp(next_token);
    }
};

#endif
