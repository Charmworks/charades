#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
#include "lp.h" // Included for LPToken definition

#include "pe_queue.h"

//#define DEBUG_PE(format, ...) { CkPrintf("PE[%d] "format, CkMyPe(), ## __VA_ARGS__); }
#define DEBUG_PE(format, ...) {}

class LP;
class LPToken;
struct tw_rng;

using std::vector;

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);

class PE: public CBase_PE {
  private:
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */
    PEQueue oldest_lps; /**< queue storing LPTokens ordered by oldest fossil */

    Time gvt;     /**< current gvt on this PE */
    int gvt_cnt;  /**< count since last gvt */
    bool waiting_on_qd;

    tw_rng * rng; /**< ROSS rng stream */

    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */
  public:
    Globals* globals;       /**< global variables accessed with PE_VALUE */
    Statistics* statistics; /**< statistics variables accessed with PE_STATS */

    PE(CProxy_Initialize);

    ~PE() {
      free(globals);
      delete statistics;
    }

    /** \brief Initialize the RNG streams for this PE */
    void initialize_rand(CProxy_Initialize);

    /** \brief Called as a reduction by LPs when load balancing is complete */
    void resume_scheduler();

    /** \brief Print final stats and end the simulation */
    void end_simulation(CkReductionMsg *m);

    /** \brief Various schedulers
        sequential: single PE, run to end
        conservative: find next epoch, assume a lookahead, run to end of epoch
        optimistic: execute events, rollback as needed, compute GVT periodically
      */
    void execute_seq();
    void execute_cons();
    void execute_opt();

    bool schedule_next_lp(); /**< call execute_me on the next LP */

    /** \brief Methods only used in optimistic mode */
    void collect_fossils();       /**< collect fossils */
    void process_cancel_q();      /**< process the cancel_q */
    void add_to_cancel_q(LP*);    /**< add an LP to the cancel_q */
    void update_min_cancel(Time); /**< update min_cancel_time */

    /** \brief Methods for GVT computation
        GVT is only used in conservative and optimistic
        In conservative it is equivalent to finding the next epoch
      */
    void gvt_begin(); /**< begin gvt computation */
    void gvt_contribute(); /**< all sent messages received, contribute to GVT */
    void gvt_end(Time); /**< gvt done, either restart the scheduler or end */
    void gvt_print(Time);

    /** \brief Get time stamp of the minium event */
    Time get_min_time();

    /** \brief Register the given LP to our queues */
    void register_lp(LPToken* next_token, Time next_ts,
                     LPToken* oldest_token, Time oldest_ts) {
      next_lps.insert(next_token, next_ts);
      oldest_lps.insert(oldest_token, oldest_ts);
    }

    /** \brief Unregister the given LP from our queues */
    void unregister_lp(LPToken* next_token, LPToken* oldest_token) {
      next_lps.remove(next_token);
      oldest_lps.remove(next_token);
      vector<LP*>::iterator it = cancel_q.begin();
      while (it != cancel_q.end()) {
        if (*it == next_token->lp) {
          cancel_q.erase(it);
          break;
        }
        it++;
      }
    }

    /** \brief Update the entry for a given LP in the next_lps */
    void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }

    /** \brief Update the entry for a given LP in the oldest_lps */
    void update_oldest(LPToken* token, Time ts) {
      oldest_lps.update(token, ts);
    }
};

#endif
