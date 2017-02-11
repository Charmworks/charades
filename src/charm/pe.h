#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
#include "scheduler.h" // Only temporary maybe

extern CProxy_PEManager pe_manager_proxy;
class LPToken;
class LP;
struct tw_rng;

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);
extern CkReduction::reducerType statsReductionType;

struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;
};

class PEManager : public CBase_PEManager {
  PEManager_SDAG_CODE
  private:
    GVTManager* gvt_manager;
    Scheduler* scheduler;
    /** Timer variables */
    double start_time;  /**< Start wall time for the simulation */
    double end_time;    /**< End wall time for the simulation */

    /** Misc variables */
    tw_rng * rng; /**< ROSS rng stream */

  public:
    Globals* globals;             /**< "global" variables per PE */
    Statistics* current_stats;    /**< statistics for the current GVT period */
    Statistics* cumulative_stats; /**< statistics for the whole run */

    PEManager();
    ~PEManager() {
      delete globals;
      delete current_stats;
      delete cumulative_stats;
    }
    void finalize(CkReductionMsg *m);

    void start_simulation();
    void end_simulation();
    void initialize_rand();
    void log_stats();

    /** TODO: Most of the following should be moved to PE manager? */
    void register_lp(LPToken* next_token, Time next_ts,
                     LPToken* oldest_token, Time oldest_ts) {
      scheduler->register_lp(next_token, next_ts, oldest_token, oldest_ts);
    }

    // TODO: Make this work
    void unregister_lp(LPToken* next_token, LPToken* oldest_token) {
      scheduler->unregister_lp(next_token, oldest_token);
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
      scheduler->update_next(token, ts);
    }
    void update_oldest(LPToken* token, Time ts) {
      scheduler->update_oldest(token, ts);
    }
    void add_to_cancel_q(LP* lp) {}
    void update_min_cancel(Time ts) {}

    void consume(RemoteEvent* e) {}
    void produce(RemoteEvent* e) {}
};

#endif
