#ifndef _GVTMANAGER_H
#define _GVTMANAGER_H

#include "gvtmanager.decl.h"
#include <float.h>

extern CProxy_GVTManager gvt_manager_proxy;

struct GVT {
  GVT() : ts(DBL_MAX), type(0) {}
  Time ts;
  unsigned type;
};

class RemoteEvent;
class PEManager;
class Scheduler;

class GVTManager : public CBase_GVTManager {
  protected:
    /** The current GVT */
    Time gvt;

    /** Local pointers to other PE-level objects */
    PEManager* pe_manager;
    Scheduler* scheduler;

  public:
    GVTManager();

    /** Every subclass needs to implement gvt_begin() */
    virtual void gvt_begin() {
      CkAbort("Need to instantiate a concrete GVT Manager\n");
    }

    /** Accessor for gvt */
    Time current_gvt() const { return gvt; }

    /** Called by the local PEManager after all groups have been initialized */
    void set_local_pointers(PEManager* pem, Scheduler* sched) {
      pe_manager = pem;
      scheduler = sched;
    }

    virtual void consume(RemoteEvent* e) {}
    virtual void produce(RemoteEvent* e) {}
}; 

class SyncGVT : public CBase_SyncGVT {
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
