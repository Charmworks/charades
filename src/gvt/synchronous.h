#ifndef _SYNCHRONOUS_H_
#define _SYNCHRONOUS_H_

#include "gvtmanager.h"

class SyncGVT : public CBase_SyncGVT {
  private:
    double gvt_start;
  public:
    SyncGVT();

    /** Starts QD */
    void gvt_begin();
    /** Called after QD is detected to contribute min time to all reduce */
    void gvt_contribute();
    /** Called by the all reduce from gvt_contribute() with resulting gvt */
    void gvt_end(Time);
};

#endif
