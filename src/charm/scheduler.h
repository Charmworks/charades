#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "scheduler.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
#include "lp.h"

#include "pe_queue.h"

class LP;
class LPToken;
struct tw_rng;


using std::vector;

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);
CkReductionMsg *gvtReduction(int nMsg, CkReductionMsg **msgs);

// Bit masks for GVT types
#define MEM_FORCE 1
#define END_FORCE 2
#define EVENT_FORCE 4

struct GVT {
  GVT() : ts(DBL_MAX), type(0) {}
  Time ts;
  unsigned type;
};

struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;

};

class Scheduler : public CBase_Scheduler {
  protected:
    /** LP queue variables */
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */
    PEQueue oldest_lps; /**< queue storing LPTokens ordered by oldest fossil */

    /** GVT variables */
    Time gvt;           /**< current GVT */
    int gvt_num;        /**< Current GVT number */
    int iter_cnt;       /**< iteration count since last gvt */

    /** Load balancing variables */
    int ldb_cnt;        /**< number of times we've called load balancing */

    /** Timer variables */
    double start_time;  /**< Start wall time for the simulation */
    double end_time;    /**< End wall time for the simulation */
#ifdef CMK_TRACE_ENABLED
    double gvt_start, ldb_start;
#endif

    /** Event cancellation variables */
    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

    /** Misc variables */
    tw_rng * rng; /**< ROSS rng stream */

  public:
    Globals* globals;             /**< "global" variables per PE */
    Statistics* current_stats;    /**< statistics for the current GVT period */
    Statistics* cumulative_stats; /**< statistics for the whole run */

    Scheduler();
    Scheduler(CProxy_Initialize);
    ~Scheduler() {
      delete globals;
      delete current_stats;
      delete cumulative_stats;
    }

    void initialize_rand();

    virtual void execute();
    virtual void gvt_done(Time gvt);

    bool schedule_next_lp(); /**< call execute_me on the next LP */

    void collect_fossils();       /**< collect fossils */
    void process_cancel_q();      /**< process the cancel_q */
    void add_to_cancel_q(LP*);    /**< add an LP to the cancel_q */
    void update_min_cancel(Time); /**< update min_cancel_time */

    void gvt_begin(); /**< begin gvt computation */
    void gvt_contribute(); /**< all sent messages received, contribute to GVT */
    void gvt_end(CkReductionMsg*); /**< gvt done, either restart the scheduler or end */

    void end_simulation(CkReductionMsg *m);

    Time get_min_time() const;

    void register_lp(LPToken* next_token, Time next_ts,
                     LPToken* oldest_token, Time oldest_ts) {
      next_lps.insert(next_token, next_ts);
      oldest_lps.insert(oldest_token, oldest_ts);
    }

    void unregister_lp(LPToken* next_token, LPToken* oldest_token) {
      next_lps.remove(next_token);
      oldest_lps.remove(oldest_token);
      vector<LP*>::iterator it = cancel_q.begin();
      while (it != cancel_q.end()) {
        if (*it == next_token->lp) {
          cancel_q.erase(it);
          break;
        }
        it++;
      }
    }

    void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }

    void update_oldest(LPToken* token, Time ts) {
      oldest_lps.update(token, ts);
    }

    void produce(RemoteEvent* msg) {}

    void consume(RemoteEvent* msg) {}
};

class ConservativeScheduler : public CBase_ConservativeScheduler {
  public:
    ConservativeScheduler(CProxy_Initialize proxy);
    void execute();
};

#endif
