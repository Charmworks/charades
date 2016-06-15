#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
#include "lp.h" // Included for LPToken definition

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
class PE: public CBase_PE {
  private:
    PEQueue next_lps;   /**< queue storing LPTokens ordered by next execution */
    PEQueue oldest_lps; /**< queue storing LPTokens ordered by oldest fossil */

    Time gvt;           /**< current gvt on this PE */
    Time min_sent;      /**< minimum ts sent out during this phase */
    Time leash_start;   /**< start of the current leash (usually == gvt) */
    int gvt_cnt;        /**< iteration count since last gvt */
    int ldb_cnt;        /**< number of times we've called load balancing */
    bool gvt_started;
    bool waiting_on_gvt;/**< flag to make sure we don't overlap gvts */
    unsigned force_gvt; /**< Bitmap used to determine if a gvt was forced */

    tw_rng * rng; /**< ROSS rng stream */

    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

    MemUsage mem_usage; //holds stats about total memory usage. data collected during gvts
#ifdef CMK_TRACE_ENABLED
    double gvt_start, ldb_start;
#endif

    // Completion detection variables for current phase, proxies, and pointers.
    unsigned current_phase, next_phase, max_phase;
    bool* detector_ready;
    CProxy_CompletionDetector* detector_proxies;
    CompletionDetector** detector_pointers;

    bool gvt_ready() const;
  public:
    Globals* globals;       /**< global variables accessed with PE_VALUE */
    Statistics* statistics; /**< statistics variables accessed with PE_STATS */

    PE(CProxy_Initialize);

    ~PE() {
      free(globals);
      delete statistics;
    }

    /** \brief Called as a reduction by LPs when load balancing is complete */
    void load_balance_complete();
    void resume_scheduler();

    /** \brief Initialize the completion detectors and CDs for this PE */
    void broadcast_detector_proxies(int num, CProxy_CompletionDetector*);
    void initialize_detectors();
    void initialize_rand();

    /** \brief Print final stats and end the simulation */
    void end_simulation(CkReductionMsg *m);

    /** \brief Various schedulers
        sequential: single PE, single chare, run to end
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
    void greedy_gvt_begin(); /**< attempt to greedily begin gvt computation */
    void gvt_begin(); /**< begin gvt computation */
    void gvt_contribute(); /**< all sent messages received, contribute to GVT */
    void gvt_end(CkReductionMsg*); /**< gvt done, either restart the scheduler or end */
    void gvt_print(GVT*);

    /** \brief Get time stamp of the minium event */
    Time get_min_time() const;

    /** \brief Register the given LP to our queues */
    void register_lp(LPToken* next_token, Time next_ts,
                     LPToken* oldest_token, Time oldest_ts) {
      next_lps.insert(next_token, next_ts);
      oldest_lps.insert(oldest_token, oldest_ts);
    }

    /** \brief Unregister the given LP from our queues */
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

    /** \brief Update the entry for a given LP in the next_lps */
    void update_next(LPToken* token, Time ts) {
      next_lps.update(token, ts);
    }

    /** \brief Update the entry for a given LP in the oldest_lps */
    void update_oldest(LPToken* token, Time ts) {
      oldest_lps.update(token, ts);
    }

    void produce(RemoteEvent* msg) {
      if (max_phase) {
        if (msg->ts < min_sent) {
          min_sent = msg->ts;
        }
        msg->phase = current_phase;
        detector_pointers[current_phase]->produce();
      }
    }

    void consume(RemoteEvent* msg) {
      if (max_phase) {
        detector_pointers[msg->phase]->consume();
      }
    }
    void add_memory_stats() {
      //Event stats
      PE_STATS(s_max_allocated) = PE_VALUE(event_buffer)->memory_stats.max_allocated;
      PE_STATS(s_avg_max_allocated) = PE_STATS(s_max_allocated);
      PE_STATS(s_remote_deallocated) = PE_VALUE(event_buffer)->memory_stats.remote_deallocated;
      PE_STATS(s_remote_new_allocated) = PE_VALUE(event_buffer)->memory_stats.remote_new_allocated;
      //Total Memory stats
      PE_STATS(s_max_memory) = mem_usage.max_memory;
      PE_STATS(s_avg_memory) = mem_usage.avg_memory / PE_STATS(s_ngvts);
    }
    void add_mem_usage(); 
    
};

#endif
