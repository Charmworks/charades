#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "scheduler.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
#include "gvtmanager.h"
#include "lp.h"

#include "pe_queue.h"

class LP;
class LPToken;
struct tw_rng;

using std::vector;

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);

struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;

};

extern CProxy_Scheduler scheduler;

class Scheduler : public CBase_Scheduler {
  protected:
    /** LP queue variables */
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */
    PEQueue oldest_lps; /**< queue storing LPTokens ordered by oldest fossil */

    /** Load balancing variables */
    //int ldb_cnt;        /**< number of times we've called load balancing */

    /** Timer variables */
    double start_time;  /**< Start wall time for the simulation */
    double end_time;    /**< End wall time for the simulation */

    /** Misc variables */
    tw_rng * rng; /**< ROSS rng stream */

  public:
    Globals* globals;             /**< "global" variables per PE */
    Statistics* current_stats;    /**< statistics for the current GVT period */
    Statistics* cumulative_stats; /**< statistics for the whole run */

    Scheduler();
    ~Scheduler() {
      delete globals;
      delete current_stats;
      delete cumulative_stats;
    }

    void initialize();
    void initialize_rand();

    virtual void execute();
    virtual void gvt_resume();
    virtual void gvt_done(Time gvt);

    bool schedule_next_lp(); /**< call execute_me on the next LP */

    void end_simulation(CkReductionMsg *m);

    virtual Time get_min_time() const;

    void register_lp(LPToken* next_token, Time next_ts,
                     LPToken* oldest_token, Time oldest_ts) {
      next_lps.insert(next_token, next_ts);
      oldest_lps.insert(oldest_token, oldest_ts);
    }

    // TODO: Make this work
    void unregister_lp(LPToken* next_token, LPToken* oldest_token) {
      /*next_lps.remove(next_token);
      oldest_lps.remove(oldest_token);
      vector<LP*>::iterator it = cancel_q.begin();
      while (it != cancel_q.end()) {
        if (*it == next_token->lp) {
          cancel_q.erase(it);
          break;
        }
        it++;
      }*/
    }

    void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }

    void update_oldest(LPToken* token, Time ts) {
      oldest_lps.update(token, ts);
    }

    // TODO: What to do with these? Only needed for some sched/GVT
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

class ConservativeScheduler : public CBase_ConservativeScheduler {
  public:
    ConservativeScheduler();
    void execute();
};

class OptimisticScheduler : public CBase_OptimisticScheduler {
  private:
    int iter_cnt;       /**< iteration count since last gvt */
    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

  public:
    OptimisticScheduler();
    void execute();
    void collect_fossils();       /**< collect fossils */
    void process_cancel_q();      /**< process the cancel_q */
    void add_to_cancel_q(LP*);    /**< add an LP to the cancel_q */
    void update_min_cancel(Time); /**< update min_cancel_time */
    Time get_min_time() const;
};

#endif
