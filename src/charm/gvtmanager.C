#include "gvtmanager.h"

#include "charm_functions.h"
#include "scheduler.h"
#include "globals.h"
 
CProxy_GVTManager gvt_manager_proxy;
CkReduction::reducerType gvtReductionType;

/* NON-MEMBER functions */
CkReductionMsg *gvtReduction(int nMsg, CkReductionMsg **msgs) {
  GVT* new_gvt = new GVT;
  for (int i = 0; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(GVT));
    GVT* gvt = (GVT*)msgs[i]->getData();
    new_gvt->ts = fmin(new_gvt->ts, gvt->ts);
    new_gvt->type = new_gvt->type | gvt->type;
  }
  return CkReductionMsg::buildNew(sizeof(GVT), new_gvt);
}

void registerGVTReduction(void) {
  gvtReductionType = CkReduction::addReducer(gvtReduction);
}

/* GVTManager FUNCTIONS */
GVTManager::GVTManager() : curr_gvt(0.0), prev_gvt(0.0) {
  contribute(CkCallback(CkReductionTarget(Scheduler, gvtManagerReady), scheduler_proxy));
}

/* GVT SYNC FUNCTIONS */
SyncGVT::SyncGVT() {
  gvt_name = "Synchronous GVT";
}

void SyncGVT::gvt_begin() {
  if(CkMyPe() == 0) {
    CkStartQD(CkCallback(CkIndex_SyncGVT::gvt_contribute(), thisProxy));
  }
}

void SyncGVT::gvt_contribute() {
  Time min_time = scheduler->get_min_time();
  CkAssert(min_time >= curr_gvt);
  
  contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(SyncGVT,gvt_end),thisProxy));

  if(g_tw_async_reduction) {
    scheduler->gvt_resume();
  }
}

void SyncGVT::gvt_end(Time new_gvt) {
  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;
  scheduler->gvt_done(curr_gvt);
}

/* Continuous GVT FUNCTIONS */

PhaseGVT::PhaseGVT() {
  gvt_name = "Two-Phase GVT";
  initialize_detectors();
}

void PhaseGVT::initialize_detectors() {
  //max_phase = g_tw_gvt_phases;
  max_phase = 2;
  current_phase = next_phase = 0;
  detector_ready = new bool[max_phase];
  detector_proxies = new CProxy_CompletionDetector[max_phase];
  detector_pointers = new CompletionDetector*[max_phase];
  for (int i = 0; i < max_phase; i++) {
    detector_ready[i] = false;
    detector_pointers[i] = NULL;
    if (CkMyPe() == 0) {
      detector_proxies[i] = CProxy_CompletionDetector::ckNew();
    }
  }
  if (CkMyPe() == 0) {
    thisProxy.broadcast_detector_proxies(max_phase, detector_proxies);
  }
}

void PhaseGVT::broadcast_detector_proxies(int num, CProxy_CompletionDetector* proxies) {
  for (int i = 0; i < num; i++) {
    detector_proxies[i] = proxies[i];
    detector_pointers[i] = detector_proxies[i].ckLocalBranch();
    detector_ready[i] = true;
    if (CkMyPe() == 0) {
      detector_proxies[i].start_detection(CkNumPes(),
          CkCallback(),
          CkCallback(),
          CkCallback(CkIndex_PhaseGVT::gvt_contribute(), thisProxy), 0);
    }
  }
  current_phase = 0;
  next_phase = (current_phase + 1) % max_phase;
}

void PhaseGVT::gvt_begin() {
  if(detector_ready[next_phase]) {
    min_sent = DBL_MAX;
    detector_pointers[current_phase]->done();
    detector_ready[current_phase] = false;
    current_phase = next_phase;
    next_phase = (current_phase+1)%max_phase;
  }
  scheduler->gvt_resume(); 
}

void PhaseGVT::gvt_contribute() {
  Time min_time = scheduler->get_min_time();
  min_time = fmin(min_time, min_sent); 
  CkAssert(min_time >= curr_gvt);
  
  if (CkMyPe() == 0) {
    detector_proxies[next_phase].start_detection(CkNumPes(),
        CkCallback(),
        CkCallback(),
        CkCallback(CkIndex_PhaseGVT::gvt_contribute(), thisProxy), 0);
  }
  contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(PhaseGVT,gvt_end),thisProxy));
}

void PhaseGVT::gvt_end(Time new_gvt) {
  detector_ready[next_phase] = true;
  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;
  scheduler->gvt_done(curr_gvt);
}

void PhaseGVT::consume(RemoteEvent* e) {
  detector_pointers[e->phase]->consume();
}

void PhaseGVT::produce(RemoteEvent* e) {
  if(e->ts < min_sent) {
    min_sent = e->ts; 
  }
  e->phase = current_phase;
  detector_pointers[current_phase]->produce();
}

#include "gvtmanager.def.h"
