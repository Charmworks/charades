#include "phased.h"

#include "charm_functions.h"
#include "event.h"
#include "scheduler.h"
#include "globals.h"
#include <float.h>

PhaseGVT::PhaseGVT() {
  gvt_name = "Multi-Phase GVT";
  
  max_phase = g_tw_gvt_phases;
  producing_phase = 0;
  next_phase = (producing_phase + 1) % max_phase;
  gvt_phase_begin = -1;
  gvt_phase_end = -1;
  detector_ready = new bool[max_phase];
  sent = new int[max_phase];
  received = new int[max_phase];
  min_sent = new Time[max_phase];
  for (int i = 0; i < max_phase; i++) {
    detector_ready[i] = true;
    sent[i] = 0;
    received[i] = 0;
    min_sent[i] = DBL_MAX;
  }
}

void PhaseGVT::gvt_begin() {

  if(detector_ready[next_phase]) {

    min_sent[producing_phase] = DBL_MAX;
    detector_ready[producing_phase] = false;
    if(gvt_phase_begin == -1) {
      gvt_phase_begin = producing_phase;
      gvt_phase_end = producing_phase;
    }
    else gvt_phase_end = (gvt_phase_end+1) % max_phase;
    producing_phase = next_phase;
    next_phase = (producing_phase+1)%max_phase;

    int counts[2] = {0,0};

    for(int i = gvt_phase_begin; i!= gvt_phase_end; i = (i+1)%max_phase) {
      counts[0]+= sent[i];
      counts[1]+= received[i];
    }
    counts[0]+= sent[gvt_phase_end];
    counts[1]+= received[gvt_phase_end];
    contribute(2 * sizeof(int), counts, CkReduction::sum_int,
      CkCallback(CkReductionTarget(PhaseGVT,check_counts),thisProxy));
  }
  else {
  //TODO Add code here for forcing gvts or handling too many gvts?

  }
  scheduler->gvt_resume();

}

void PhaseGVT::check_counts(int s, int r) {

  if(s == r) {

    Time min_time = scheduler->get_min_time();

    min_time = fmin(min_time, min_sent[gvt_phase_end]);
    CkAssert(min_time >= curr_gvt);

    contribute(sizeof(Time), &min_time, CkReduction::min_double,
      CkCallback(CkReductionTarget(PhaseGVT,gvt_end),thisProxy));
  }

  else {
    int counts[2] = {0,0};

    for(int i = gvt_phase_begin; i!= gvt_phase_end; i = (i+1)%max_phase) {
      counts[0]+= sent[i];
      counts[1]+= received[i];
    }
    counts[0]+= sent[gvt_phase_end];
    counts[1]+= received[gvt_phase_end];

    contribute(2 * sizeof(int), counts, CkReduction::sum_int,
      CkCallback(CkReductionTarget(PhaseGVT,check_counts),thisProxy));
  }

}

void PhaseGVT::gvt_end(Time new_gvt) {

  prev_gvt = curr_gvt;
  curr_gvt = new_gvt;
  for(int i = gvt_phase_begin; i!= gvt_phase_end; i = (i+1)%max_phase) {
    detector_ready[i] = true;
    sent[i] = 0;
    received[i] = 0;
  }
  detector_ready[gvt_phase_end] = true;
  sent[gvt_phase_end] = 0;
  received[gvt_phase_end] = 0;

  if( (gvt_phase_end + 1)% max_phase  == producing_phase) {
    gvt_phase_begin = -1;
    gvt_phase_end = -1;
  }
  else {
   DEBUG_PE("Error? \n");
  }
  scheduler->gvt_done(curr_gvt);
}

void PhaseGVT::consume(RemoteEvent* e) {
  received[e->phase]++;
}

void PhaseGVT::produce(RemoteEvent* e) {
  if (gvt_phase_end != -1) {
    if(e->ts < min_sent[gvt_phase_end]) {
      min_sent[gvt_phase_end] = e->ts;
    }
  }
  e->phase = producing_phase;
  sent[producing_phase]++;
}
