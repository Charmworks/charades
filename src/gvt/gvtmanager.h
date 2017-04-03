#ifndef _GVTMANAGER_H
#define _GVTMANAGER_H

#include "gvtmanager.decl.h"

extern CProxy_GVTManager gvt_manager_proxy;

class RemoteEvent;
class DistributedScheduler;

class GVTManager : public CBase_GVTManager {
  protected:
    /** Global virtual times */
    Time curr_gvt, prev_gvt;

    /** Local pointers to other PE-level objects */
    DistributedScheduler* scheduler;

    /** GVT name for output purposes */
    std::string gvt_name;

  public:
    GVTManager();

    /** Every subclass needs to implement gvt_begin() */
    virtual void gvt_begin() { CkAbort("Cannot call GVTManager::begin()\n"); }

    /** Accessors for gvts */
    Time current_gvt() const { return curr_gvt; }
    Time previous_gvt() const { return prev_gvt; }

    /** Called by the local PEManager after all groups have been initialized */
    virtual void groups_created();

    /** Methods for producing and consuming events for GVTs that need to know */
    virtual void consume(RemoteEvent* e) {}
    virtual void produce(RemoteEvent* e) {}
};

#endif
