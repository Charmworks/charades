#ifndef _PHASED_H_
#define _PHASED_H_

#include "gvtmanager.h"

class PhaseGVT : public CBase_PhaseGVT {
  public:

    PhaseGVT();
    void finalize();
    /** Switch phases if next phase ready and start GVT process for current phase**/
    void gvt_begin();
    /** Check if phase has completed detection, if so contribute min time to all reduce**/
    void check_counts(int, int);
    /** Called by the all reduce from check_counts() with resulting gvt**/
    void gvt_end(Time);
    /**Increment received count for the phase of the event **/
    void consume(RemoteEvent* e);
    /**Increment sent count for producing phase and recalculate min_sent**/
    bool produce(RemoteEvent* e);

  private:

    unsigned max_phase, producing_phase, active_phase;
    uint32_t* sent;
    uint32_t* received;
    Time  min_sent;

    // Reduction statistics
    uint32_t count_reductions, min_reductions;
    // Begin statistics
    uint32_t begin_successes, begin_fails;
};

#endif
