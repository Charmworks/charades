#include "cd.h"

#include "completion.h"
#include "event.h"
#include "scheduler.h"
#include "globals.h"
#include <float.h>

CdGVT::CdGVT() {
  gvt_name = "Two-Phase GVT";
  active = false;
  continuous = true;

  initialize_detectors();
}

void CdGVT::initialize_detectors() {
  max_phase = g_tw_gvt_phases;
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

void CdGVT::broadcast_detector_proxies(int num, CProxy_CompletionDetector* proxies) {
  for (int i = 0; i < num; i++) {
    detector_proxies[i] = proxies[i];
    detector_pointers[i] = detector_proxies[i].ckLocalBranch();
    detector_ready[i] = true;
  }
  current_phase = 0;
  next_phase = (current_phase + 1) % max_phase;
}

void CdGVT::gvt_begin() {
  if(detector_ready[next_phase]) {
    active = true;
    if (CkMyPe() == 0) {
      detector_proxies[current_phase].start_detection(CkNumPes(),
          CkCallback(),
          CkCallback(),
          CkCallback(CkIndex_CdGVT::gvt_contribute(), thisProxy), 0);
    }
    min_sent = TIME_MAX;
    detector_pointers[current_phase]->done();
    detector_ready[current_phase] = false;
    current_phase = next_phase;
    next_phase = (current_phase+1)%max_phase;
  }
  scheduler->gvt_resume();
}

void CdGVT::gvt_contribute() {
  Time min_time = scheduler->get_min_time();
  min_time = std::min(min_time, min_sent);
  CkAssert(min_time >= curr_gvt);
  contribute(sizeof(Time), &min_time, CkReduction::min_ulong_long,
      CkCallback(CkReductionTarget(CdGVT,gvt_end),thisProxy));
}

void CdGVT::gvt_end(Time new_gvt) {
  // TODO: Why is ready not set to true in contribute?
  active = false;
  detector_ready[next_phase] = true;
  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;
  scheduler->gvt_done(curr_gvt);
}

void CdGVT::consume(RemoteEvent* e) {
  detector_pointers[e->phase]->consume();
}

bool CdGVT::produce(RemoteEvent* e) {
  if(e->ts < min_sent) {
    min_sent = e->ts;
  }
  e->phase = current_phase;
  detector_pointers[current_phase]->produce();
  return true;
}
