#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"
#include "pe_queue.h"

// Included for LPToken
#include "charm/lp.h"

#include "globals.h"

class LP;

class PE: public CBase_PE {
  private:
    PEQueue nextEvents; /**< queue to store the time stamp for next events that an LP has to execute*/
    PEQueue oldestEvents; /**< queue to store the time stamp for the earliest event that an LP has execute beyond the last computed GVT*/
    Time gvt, currTime, endTime; /**< current time on this PE */
    Time lookahead; /**< look ahead of conservative simulation */
    int batchSize; /**< number of events executed before network polling */
    int gvt_cnt; /**< count since last gvt */
    int gvt_freq; /**< frequency of GVT computation */
  public:
    // A struct of global variables stored on each PE.
    Globals* globals;
    std::vector<LP*> cancel_q;

    // TODO: Commented this out temporarily so that code would compile
    //PE() { this->PE(default_batchSize, default_gvt_freq); }

    PE(int batchSize_, int gvt_freq_) : batchSize(batchSize_), gvt_cnt(0), gvt_freq(gvt_freq_) { }

    /** \brief Various schedulers
        sequential (no communication, run to end)
        conservative (find the next epoch, assume a lookahead, run to end of epoch
        optimistic (run till you are forced to rollback, compute GVT once in a while)
      */
    void execute_seq();
    void execute_cons();
    void execute_opt();
    void collect_fossils(); /**< collect fossils */
    int schedule_nextLP(); /**< find the smallest time step and execute */
    void process_cancel_q();

    /** \brief Methods for GVT computation */
    void GVT_begin(); /**< begin gvt computation*/
    void GVT_contribute(); /**< all sent messages received, contribute to GVT */
    void GVT_end(Time); /**< GVT computed */

    /** \brief Get time stamp of the minium event */
    Time getMinTime() {
      return nextEvents.top()->ts;
    }

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
