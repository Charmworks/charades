#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"

#include "pe_queue.h"

//#include "completion.h"

class LP;
class LPToken;
struct tw_rng;

using std::vector;

class PE: public CBase_PE {
  private:
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */
    PEQueue oldest_lps; /**< queue storing LPTokens ordered by oldest fossil */

    Time gvt;     /**< current gvt on this PE */
    int gvt_cnt;  /**< count since last gvt */

    tw_rng * rng; /**< ROSS rng stream */

    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

    // Completion detection variables for current phase, proxies, and pointers.
    unsigned current_phase, next_phase;
    bool detector_ready[2];
    CProxy_CompletionDetector detector_proxies[2];
    CompletionDetector* detector_pointers[2];
  public:
    Globals* globals;       /**< global variables accessed with PE_VALUE */
    Statistics* statistics; /**< statistics variables accessed with PE_STATS */

    PE(CProxy_Initialize);

    ~PE() {
      delete globals;
      delete statistics;
    }

    void initialize_rand();
    void initialize_detector();
    void detector_started();
    void broadcast_detector_proxies(CProxy_CompletionDetector*);

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

    /** \brief Print final stats at the end of a simulation */
    void print_final_stats(double);

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
    }

    /** \brief Update the entry for a given LP in the next_lps */
    void update_next(LPToken* token, Time ts) {
        next_lps.update(token, ts);
    }

    /** \brief Update the entry for a given LP in the oldest_lps */
    void update_oldest(LPToken* token, Time ts) {
        oldest_lps.update(token, ts);
    }

    void produce(RemoteEvent* msg) {
      msg->phase = current_phase;
      detector_pointers[current_phase]->produce();
    }

    void consume(RemoteEvent* msg) {
      detector_pointers[msg->phase]->consume();
    }
};

#endif
