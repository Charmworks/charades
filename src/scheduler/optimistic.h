#ifndef _OPTIMISTIC_H_
#define _OPTIMISTIC_H_

#include "scheduler.h"

#include "lp.h" // Included for LPToken

using std::vector;

class OptimisticScheduler : public CBase_OptimisticScheduler {
  private:
    // TODO: Do we need a separate cancel queue, or can we treat it like FC?
    Time min_cancel_time; /**< minumum event time in the cancel queue */
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

  public:
    OptimisticScheduler();
    void execute();

    /** Method overrides for GVT control flow */
    void gvt_resume();        /**< Overriden to allow overlap with GVT */
    void gvt_done(Time gvt);  /**< Partially overridden to add FC */

    /** Extra methods for dealing with speculative events */
    void collect_fossils(Time gvt); /**< Free/commit events before gvt */
    void process_cancel_q();        /**< Process the LPs in the cancel_q*/
    void add_to_cancel_q(LP*);      /**< Add an LP to the cancel_q */
    void update_min_cancel(Time);   /**< Update min_cancel_time */
    Time get_min_time() const;      /**< Overriden to include min_cancel_time */

    // TODO: What does it mean to remove from cancel queue if it's actually in
    // the cancel queue?
    /** Partially overridden to deal with cancel queue */
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
