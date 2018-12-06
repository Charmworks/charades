#include "phased.h"

#include "event.h"
#include "scheduler.h"
#include "globals.h"
#include "util.h"

#include <float.h>
#include <iostream>

PhaseGVT::PhaseGVT() {
  gvt_name = "Multi-Phase GVT";
  active = false;
  continuous = true;
  
  max_phase = g_tw_gvt_phases;
  producing_phase = 0;
  active_phase = -1;

  min_sent = DBL_MAX;
  sent = new uint32_t[max_phase];
  received = new uint32_t[max_phase];
  for (uint32_t i = 0; i < max_phase; i++) {
    sent[i] = 0;
    received[i] = 0;
  }

  count_reductions = min_reductions = 0;
}

void PhaseGVT::finalize() {
  if (CkMyPe() == 0) {
    std::cout << std::endl;
    std::cout << "==== PHASE GVT REDUCTION STATISTICS ========" << std::endl;
    std::cout << "Count Reductions : " << count_reductions << std::endl;
    std::cout << "Min Reductions   : " << min_reductions << std::endl;
    std::cout << "Total Reductions : " << count_reductions + min_reductions;
  }
}

void PhaseGVT::gvt_begin() {
  if(!active) {
    active = true;
    min_sent = DBL_MAX;

    active_phase = producing_phase;
    producing_phase = (producing_phase + 1) % max_phase;

    uint32_t counts[] = { sent[active_phase], received[active_phase] };
    contribute(2 * sizeof(uint32_t), counts, CkReduction::sum_int,
      CkCallback(CkReductionTarget(PhaseGVT,check_counts),thisProxy));
  }
  scheduler->gvt_resume();
}

void PhaseGVT::check_counts(int s, int r) {
  count_reductions++;

  if(s == r) {
    Time min_time = scheduler->get_min_time();
    min_time = fmin(min_time, min_sent);
    contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(PhaseGVT,gvt_end),thisProxy));
  } else {
    uint32_t counts[] = { sent[active_phase], received[active_phase] };
    contribute(2 * sizeof(uint32_t), counts, CkReduction::sum_int,
      CkCallback(CkReductionTarget(PhaseGVT,check_counts),thisProxy));
  }
}

void PhaseGVT::gvt_end(Time new_gvt) {
  min_reductions++;

  active = false;
  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;

  scheduler->gvt_done(curr_gvt);
}

void PhaseGVT::consume(RemoteEvent* e) {
  received[e->phase]++;
}

bool PhaseGVT::produce(RemoteEvent* e) {
  if(e->ts < min_sent) {
    min_sent = e->ts;
  }

  e->phase = producing_phase;
  sent[producing_phase]++;
  return true;
}
