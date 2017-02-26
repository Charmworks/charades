#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "scheduler.decl.h"

#include "typedefs.h"
#include "pe_queue.h"
#include "lp.h" // Temporary for LPToken
#include "charm_functions.h" // temporary for DEBUG_PE
#include "gvtmanager.h" // temporary for produce/consume

extern CProxy_Scheduler scheduler_proxy;

class RemoteEvent;
class LP;
class LPToken;
class PEManager;
class Globals;
class Statistics;

using std::vector;

class Scheduler : public CBase_Scheduler {
  Scheduler_SDAG_CODE
  protected:
    /** LP queue variables */
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */

    /** Local pointers to other PE-level objects */
    GVTManager* gvt_manager;

    /** Timer variables TODO: Just move to stats. */
    double start_time;  /**< Start wall time for the simulation */
    double end_time;    /**< End wall time for the simulation */

    /** Misc variables */
    tw_rng * rng; /**< ROSS rng stream */

    std::string scheduler_name;

  public:
    /** "Global" variables per PE */
    Globals* globals;
    /** Stats for the scheduler */
    Statistics* stats;

    Scheduler();

    void finalize(CkReductionMsg *m);
    void start_simulation();
    void end_simulation();

    void initialize_rand();

    /** Entry method for executing a scheduler iteration */
    virtual void execute();

    /** Local method which allows the GVT manager to signify LP execution can
     *  continue without messing up the GVT. The specific scheduler subclasses
     *  will decide whether or not anything can be done at this point. */
    virtual void gvt_resume();
    /** Local method which allows the GVT manager to signify that the GVT is
     *  comleted with the passed in GVT being the result. */
    virtual void gvt_done(Time gvt);

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
    void consume(RemoteEvent* e) {
      gvt_manager->consume(e);
    }
    void produce(RemoteEvent* e) {
      gvt_manager->produce(e);
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

// TODO: Change iteration_done to is_ready or something like that
// TODO: Add gvt_done(gvt) method and maybe one for resume as well...since this
// is the GVT trigger, signalling it when GVT completes and when iter completes
// seems reasonable
// TODO: Make scheduler templated on trigger type. Specialize constructors to
// read in trigger config from file (along with everything else it needs to read
// in). Common init can be refactored out of specialized c-tors to regular
// method.
// TODO: Leash can set current leash start to DBL max on returning true so async
// red will allow even exec
class GVTTrigger {
  public:
    virtual void iteration_done() = 0;
    virtual void gvt_done(Time gvt) = 0;
    virtual bool is_ready(Time next) const = 0;
};

class CountTrigger : public GVTTrigger {
  private:
    unsigned int iter_cnt, iter_max;
  public:
    CountTrigger(unsigned max) : iter_cnt(0), iter_max(max) {}
    void iteration_done() { iter_cnt++; }
    void gvt_done(Time gvt) {}
    bool is_ready(Time next) const {
      return (iter_cnt % iter_max == 0);
    }
};

class LeashTrigger : public GVTTrigger {
  private:
    Time leash_start, leash_length;
  public:
    LeashTrigger(Time l) : leash_start(0.0), leash_length(l) {}
    void iteration_done() {}
    void gvt_done(Time gvt) { leash_start = gvt; }
    bool is_ready(Time next) const {
      return (leash_start + leash_length <= next);
    }
};

class OptimisticScheduler : public CBase_OptimisticScheduler {
  private:
    Time min_cancel_time; /**< minumum event time in the cancel queue */
    // TODO: Don't use this...instead just do what FC does and call the method
    // on ever LP on PE. This can use the PE level list of LPs. In fact, if we
    // allow for some sort of visitor pattern we can do the same for cancel
    // processing, LB control, and FC. Should maybe even be faster because we
    // won't need to constantly mess with the vector of LPs.
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

    // TODO: Maybe this could be a template instead
    // TODO: For leash with async red or something like that the GVTMan needs a
    // leash estimate (probably in just holding local min)
    CountTrigger trigger;

  public:
    OptimisticScheduler();
    void execute();
    void gvt_resume();
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
