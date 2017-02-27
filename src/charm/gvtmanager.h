#ifndef _GVTMANAGER_H
#define _GVTMANAGER_H

#include "gvtmanager.decl.h"
#include "charm_functions.h" // Temp for DEBUG_PE
#include <float.h>

extern CProxy_GVTManager gvt_manager_proxy;

struct GVT {
  GVT() : ts(DBL_MAX), type(0) {}
  Time ts;
  unsigned type;
};

class RemoteEvent;
class Scheduler;

class GVTManager : public CBase_GVTManager {
  protected:
    /** Global virtual times */
    Time curr_gvt, prev_gvt;

    /** Local pointers to other PE-level objects */
    Scheduler* scheduler;

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
    void set_local_pointers(Scheduler* sched) { scheduler = sched; }

    /** Methods for producing and consuming events for GVTs that need to know */
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

class PhaseGVT : public CBase_PhaseGVT {
  public:
    PhaseGVT();

    void gvt_begin();

    void gvt_contribute();

    void gvt_end(Time);

    void initialize_detectors();
    void broadcast_detector_proxies(int, CProxy_CompletionDetector*);

    void consume(RemoteEvent* e);
    void produce(RemoteEvent* e);

  private:

    unsigned max_phase, current_phase, next_phase;
    bool* detector_ready;
    CProxy_CompletionDetector* detector_proxies;
    CompletionDetector** detector_pointers;

    Time min_sent;
};

#endif
