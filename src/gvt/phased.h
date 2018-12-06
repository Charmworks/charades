#ifndef _PHASED_H_
#define _PHASED_H_

#include "gvtmanager.h"

class PhaseGVT : public CBase_PhaseGVT {
  public:
    PhaseGVT();
    void finalize();
    /** Start a new GVT computation if not already active **/
    void gvt_begin();
    /** Check if sent == received, if so compute GVT **/
    void check_counts(int s, int r);
    /** Cleanup and report new GVT **/
    void gvt_end(Time);
    /** Increment received count based on events phase */
    void consume(RemoteEvent* e);
    /** Increment sent count for producing phase update min sent time **/
    bool produce(RemoteEvent* e);

  private:
    uint32_t max_phase, producing_phase, active_phase;
    uint32_t *sent;
    uint32_t *received;
    Time min_sent;

    // Reduction statistics
    uint32_t count_reductions, min_reductions;
};

#endif
