#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"

#include "typedefs.h"
#include "globals.h"
#include "statistics.h"

extern CProxy_PEManager pe_manager_proxy;
struct tw_rng;

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);
extern CkReduction::reducerType statsReductionType;

struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;
};

class PEManager : public CBase_PEManager {
  PEManager_SDAG_CODE
  private:
    GVTManager* gvt_manager;
    Scheduler* scheduler;
    /** Timer variables */
    double start_time;  /**< Start wall time for the simulation */
    double end_time;    /**< End wall time for the simulation */

    /** Misc variables */
    tw_rng * rng; /**< ROSS rng stream */

  public:
    Globals* globals;             /**< "global" variables per PE */
    Statistics* current_stats;    /**< statistics for the current GVT period */
    Statistics* cumulative_stats; /**< statistics for the whole run */

    PEManager();
    ~PEManager() {
      delete globals;
      delete current_stats;
      delete cumulative_stats;
    }
    void finalize(CkReductionMsg *m);

    void start_simulation();
    void end_simulation();
    void initialize_rand();
    void log_stats();
};

#endif
