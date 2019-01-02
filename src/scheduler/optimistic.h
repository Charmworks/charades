#ifndef _OPTIMISTIC_H_
#define _OPTIMISTIC_H_

#include "scheduler.h"

#include "lp.h" // Included for LPToken

using std::vector;

class OptimisticScheduler : public CBase_OptimisticScheduler {
  private:
    // TODO: Do we need a separate cancel queue, or can we treat it like FC?
    Time min_cancel_time; /**< minumum event time in the cancel queue */

  public:
    OptimisticScheduler();
    void execute();

    /** Method overrides for GVT control flow */
    void gvt_resume();        /**< Overriden to allow overlap with GVT */
    void gvt_done(Time gvt, bool lb = false);  /**< Partially overridden to add FC */

    /** Extra methods for dealing with speculative events */
    void collect_fossils(Time gvt); /**< Free/commit events before gvt */
    void balancing_complete();
    void process_cancel_q();        /**< Process the LPs in the cancel_q*/
    void update_min_cancel(Time);   /**< Update min_cancel_time */
    Time get_min_time() const;      /**< Overriden to include min_cancel_time */
};

#endif
