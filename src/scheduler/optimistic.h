#ifndef _OPTIMISTIC_H_
#define _OPTIMISTIC_H_

#include "scheduler.h"

#include "lp.h" // Included for LPToken

class OptimisticScheduler : public CBase_OptimisticScheduler {
  private:
    Time min_cancel_time; /**< minumum event time in the cancel queue */
    // TODO: Don't use this...instead just do what FC does and call the method
    // on ever LP on PE. This can use the PE level list of LPs. In fact, if we
    // allow for some sort of visitor pattern we can do the same for cancel
    // processing, LB control, and FC. Should maybe even be faster because we
    // won't need to constantly mess with the vector of LPs.
    vector<LP*> cancel_q; /**< list of LPs with events for cancellation */

  public:
    OptimisticScheduler();
    void execute();
    void gvt_resume();
    void gvt_done(Time gvt);
    void collect_fossils(Time gvt);       /**< collect fossils */
    void process_cancel_q();      /**< process the cancel_q */
    void add_to_cancel_q(LP*);    /**< add an LP to the cancel_q */
    void update_min_cancel(Time); /**< update min_cancel_time */
    Time get_min_time() const;    /**< Override base method to include cancel q*/

    // TODO: What does it mean to remove from cancel queue if it's actually in
    // the cancel queue?
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
