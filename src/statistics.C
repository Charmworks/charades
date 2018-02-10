/** \file statistics.C
 *  Definition of the Statistics class and associated methods/variables.
 */

#include "statistics.h"

#include "scheduler.h"

#include <cstring>  // Included for std::memcpy
#include <stdio.h>  // Included for printf
#include <float.h>  // Included for DBL_MAX
#include <math.h>   // Included for fmax/fmin

Statistics* get_statistics() {
  /** Cache the pointer in a local variable for quicker lookup */
  static Statistics* statistics = ((Scheduler*)CkLocalBranch(scheduler_id))->stats;
  return statistics;
}

CkReduction::reducerType statsReductionType;
void registerStatsReduction() {
  statsReductionType = CkReduction::addReducer(statsReduction);
}

/**
 * \param nMsg the number of messages to reduce
 * \param msgs an array of message pointers to the actual messages
 * \returns a single message that is the reduction of the passed in messages
 */
CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs) {
  CkAssert(nMsg > 0);
  CkAssert(msgs[0]->getSize() == sizeof(Statistics));
  Statistics* s = new Statistics();
  std::memcpy(s, msgs[0]->getData(), sizeof(Statistics));

  for (int i = 1; i < nMsg; i++) {
    CkAssert(msgs[i]->getSize() == sizeof(Statistics));

    Statistics* c = (Statistics*)msgs[i]->getData();
    s->reduce(c);
  }

  return CkReductionMsg::buildNew(sizeof(Statistics), s);
}

Statistics::Statistics() {
  clear();
}

void Statistics::clear() {
  // Timing stats
  total_time.clear();
#ifdef DETAILED_TIMING
  execute_time.clear();
  gvt_time.clear();
  gvt_delay.clear();
  lb_time.clear();
#endif

  // Event stats
  events_executed = 0;
  events_committed = 0;
  events_rolled_back = 0;

  // Rollback stats
  total_rollback_calls = 0;
  ts_rollback_calls = 0;
  event_rollback_calls = 0;

  // Send stats
  self_sends = 0;
  local_sends = 0;
  remote_sends = 0;
  anti_sends = 0;

  // GVT stats
  total_gvts = 0;

  // LB stats
  total_lbs = 0;

  // Memory stats
  max_events_used = 0;
  new_event_calls = 0;
  del_event_calls = 0;
}

/**
 * Just call finalize on all Stat objects so they are ready for the reduction.
 */
void Statistics::finalize() {
  total_time.finalize();
#ifdef DETAILED_TIMING
  execute_time.finalize();
  gvt_time.finalize();
  gvt_delay.finalize();
  lb_time.finalize();
#endif
}

/**
 * The accumulation done here is used when current stats are logged to the
 * tracing file, and then added to the cumulative stats for the PE. This is
 * slightly different that a reduction. One example of a difference, is that the
 * total number of GVTs is summed, where for a reduction they should be the same
 * and are left unmodified.
 */
void Statistics::add(const Statistics* other) {
  // Timing stats
  total_time.add(other->total_time);
#ifdef DETAILED_TIMING
  execute_time.add(other->execute_time);
  gvt_time.add(other->gvt_time);
  gvt_delay.add(other->gvt_delay);
  lb_time.add(other->lb_time);
#endif

  // Event stats
  events_executed += other->events_executed;
  events_committed += other->events_committed;
  events_rolled_back += other->events_rolled_back;

  // Rollback stats
  total_rollback_calls += other->total_rollback_calls;
  ts_rollback_calls += other->ts_rollback_calls;
  event_rollback_calls += other->event_rollback_calls;

  // Send stats
  self_sends += other->self_sends;
  local_sends += other->local_sends;
  remote_sends += other->remote_sends;
  anti_sends += other->anti_sends;

  // GVT stats
  total_gvts += other->total_gvts;

  // LB stats
  total_lbs += other->total_lbs;

  // Memory stats
  max_events_used = fmax(max_events_used, other->max_events_used);
  new_event_calls += other->new_event_calls;
  del_event_calls += other->del_event_calls;
}

/**
 * The accumulation done here is used when combining cumulative stats from
 * multiple PEs. So for example, the total number of GVTs is left unchanged
 * since every PE does the same number of GVTs.
 */
void Statistics::reduce(const Statistics* other) {
  // Timing stats
  total_time.reduce(other->total_time);
#ifdef DETAILED_TIMING
  execute_time.reduce(other->execute_time);
  gvt_time.reduce(other->gvt_time);
  gvt_delay.reduce(other->gvt_delay);
  lb_time.reduce(other->lb_time);
#endif

  events_executed += other->events_executed;
  events_committed += other->events_committed;
  events_rolled_back += other->events_rolled_back;

  // Rollback stats
  total_rollback_calls += other->total_rollback_calls;
  ts_rollback_calls += other->ts_rollback_calls;
  event_rollback_calls += other->event_rollback_calls;

  // Send stats
  self_sends += other->self_sends;
  local_sends += other->local_sends;
  remote_sends += other->remote_sends;
  anti_sends += other->anti_sends;

  // GVT stats
  total_gvts = other->total_gvts;

  // LB stats
  total_lbs = other->total_lbs;

  // Memory stats
  max_events_used = fmax(max_events_used, other->max_events_used);
  new_event_calls += other->new_event_calls;
  del_event_calls += other->del_event_calls;
}

/** Print a section title and border */
void Statistics::print_section(const char* name) const {
  CkPrintf("\n\n==== %-32s ==================================\n", name);
}

/** Print a formatted and labeled integer value */
void Statistics::print_int(const char* name, uint32_t v) const {
  CkPrintf("\t%-50s %11lld\n", name, v);
}

/** Print a formatted and labeled double value */
void Statistics::print_double(const char* name, double v) const {
  CkPrintf("\t%-50s %11.2f\n", name, v);
}

/** Print a formatted and labeled Stat<double> value (with min and max) */
void Statistics::print_stat_double(const char* name,
    const Stat<double>& stat) const {
  CkPrintf("\t%-50s %11.2f [%.2f,%.2f]\n",
    name, stat.total / CkNumPes(), stat.minimum, stat.maximum);
}

/** Print out the entire stats class at the end of a simulation */
void Statistics::print() const {
  /** \todo Add stats about PEs, num nodes, etc? */
  print_section("SUMMARY");
  print_int("Events Committed", events_committed);
  print_double("Execution Time", total_time.maximum);
  print_double("Event Rate", events_committed / total_time.maximum);

  print_section("EVENT STATISTICS");
  print_int("Total Events Processed", events_executed);
  print_int("Events Committed", events_committed);
  print_int("Events Rolled Back", events_rolled_back);
  print_double("Efficiency", 100.0 * ((double) events_committed / (double) events_executed));

  print_section("GVT STATISTICS");
  print_int("Total GVT Computations", total_gvts);

  print_section("LB STATISTICS");
  print_int("Total LB Calls", total_lbs);

  /** \todo Improve these stats to include remote % and also track the sent
   * events that were committed vs those sent and cancelled, or rolled back */
  print_section("SEND STATISTICS");
  print_int("Self Sends", self_sends);
  print_int("Local Sends", local_sends);
  print_int("Remote Sends", remote_sends);
  print_int("Anti Event Sends", anti_sends);

  /** \todo Add stats about cancellations */
  print_section("ROLLBACK STATISTICS");
  print_int("Total Rollback Calls ", total_rollback_calls);
  print_int("Timestamp Rollback Calls ", ts_rollback_calls);
  print_int("Event Rollback Calls ", event_rollback_calls);

  print_section("MEMORY STATISTICS");
  print_int("Max Events Allocated on a PE", max_events_used);
  print_int("Remote Event Allocations", new_event_calls);
  print_int("Remote Event Dealloactions", del_event_calls);

#ifdef DETAILED_TIMING
  print_section("DETAILED TIMING");
  print_stat_double("Total Time", total_time);
  print_stat_double("Forward Execute Time", execute_time);
  print_stat_double("GVT Time", gvt_time);
  print_stat_double("GVT Delay", gvt_delay);
  print_stat_double("LB Time", lb_time);
#endif
}

#if CMK_TRACE_ENABLED
/** Register stats with the Charm++ tracing framework */
void Statistics::init_tracing() {
    EVENTS_EXECUTED = traceRegisterUserStat("Events executed", -1);
    EVENTS_COMMITTED = traceRegisterUserStat("Events committed", -1);
    EVENTS_ROLLED_BACK = traceRegisterUserStat("Events rolled back", -1);

    EFFICIENCY = traceRegisterUserStat("Efficiency", -1);

    SELF_SENDS = traceRegisterUserStat("Self sends", -1);
    LOCAL_SENDS = traceRegisterUserStat("Local sends", -1);
    REMOTE_SENDS = traceRegisterUserStat("Remote sends", -1);
    ANTI_SENDS = traceRegisterUserStat("Anti sends", -1);

    MAX_EVENTS_USED = traceRegisterUserStat("Max event used", -1);
    NEW_EVENT_CALLS = traceRegisterUserStat("New event calls", -1);
    DEL_EVENT_CALLS = traceRegisterUserStat("Delete event calls", -1);
}

/** Update all the stat values for a particular GVT number */
void Statistics::log_tracing(int gvt_num) const {
  updateStatPair(EVENTS_EXECUTED, events_executed, gvt_num);
  updateStatPair(EVENTS_COMMITTED, events_committed, gvt_num);
  updateStatPair(EVENTS_ROLLED_BACK, events_rolled_back, gvt_num);

  updateStatPair(EFFICIENCY, 100.0 * (1.0 - ((double) events_rolled_back / (double) events_committed)), gvt_num);

  updateStatPair(SELF_SENDS, self_sends, gvt_num);
  updateStatPair(LOCAL_SENDS, local_sends, gvt_num);
  updateStatPair(REMOTE_SENDS, remote_sends, gvt_num);
  updateStatPair(ANTI_SENDS, anti_sends, gvt_num);

  updateStatPair(MAX_EVENTS_USED, max_events_used, gvt_num);
  updateStatPair(NEW_EVENT_CALLS, new_event_calls, gvt_num);
  updateStatPair(DEL_EVENT_CALLS, del_event_calls, gvt_num);
}
#endif
