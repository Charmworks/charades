#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"
#include "pe_queue.h"

// Included for LPToken
#include "lp.h"
#include "float.h"

#include "globals.h"
#include "statistics.h"
#include "ross.h"
#include "ross_random.h"

class LP;

class PE: public CBase_PE {
  private:
    PEQueue nextEvents; /**< queue to store the time stamp for next events that an LP has to execute*/
    PEQueue oldestEvents; /**< queue to store the time stamp for the earliest event that an LP has execute beyond the last computed GVT*/
    Time gvt, currTime; /**< current time on this PE */
    int gvt_cnt; /**< count since last gvt */
  public:
    // A struct of global variables stored on each PE.
    Globals* globals;
    // A sturct of statistics variables
    Statistics* statistics;

    // TODO: Do we need another queue for this?
    std::vector<LP*> cancel_q;
    tw_rng  *rng;

    PE(CProxy_Initialize);

    ~PE() {
      delete globals;
      delete statistics;
    }

    void initialize_rand(CProxy_Initialize);

    /** \brief Various schedulers
        sequential (no communication, run to end)
        conservative (find the next epoch, assume a lookahead, run to end of epoch
        optimistic (run till you are forced to rollback, compute GVT once in a while)
      */
    void execute_seq();
    void execute_cons();
    void execute_opt();
    void collect_fossils(); /**< collect fossils */
    //int schedule_nextLP_no_save(); /**< find the smallest time step and execute for non-optimistic */
    int schedule_next_LP(); /**< find the smallest time step and execute */
    void process_cancel_q();

    /** \brief Methods for GVT computation */
    void GVT_begin(); /**< begin gvt computation*/
    void GVT_contribute(); /**< all sent messages received, contribute to GVT */
    void GVT_end(Time); /**< GVT computed */
    void endExec(double);

    /** \brief Get time stamp of the minium event */
    Time getMinTime();

    /** \brief Register the given LP to our queues */
    void register_lp(LPToken* next_token, Time next_ts, LPToken* oldest_token, Time oldest_ts) {
        nextEvents.insert(next_token, next_ts);
        oldestEvents.insert(oldest_token, oldest_ts);
    }

    /** \brief Unregister the given LP from our queues */
    void unregister_lp(LPToken* next_token, LPToken* oldest_token) {
        nextEvents.remove(next_token);
        oldestEvents.remove(next_token);
    }

    /** \brief Update the entry for a given LP in the nextEvents */
    void update_next(LPToken* token, Time ts) {
        nextEvents.update(token, ts);
    }

    /** \brief Update the entry for a given LP in the oldestEvents */
    void update_oldest(LPToken* token, Time ts) {
        oldestEvents.update(token, ts);
    }
};
#endif
