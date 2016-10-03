#include "statistics.h"

#include <stdio.h>  // Included for printf
#include <float.h>  // Included for DBL_MAX
#include <math.h>   // Included for fmax/fmin

#include <charm++.h>

Statistics::Statistics() {
  clear();
}

void Statistics::clear() {
  // Timing stats
  total_time = 0.0;

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
  total_forced_gvts = 0;
  mem_forced_gvts = 0;
  end_forced_gvts = 0;
  event_forced_gvts = 0;
  fossil_collect_calls = 0;

  // Memory stats
  max_memory_used = 0;
  max_events_used = 0;
  new_event_calls = 0;
  del_event_calls = 0;
}

void Statistics::add(const Statistics* other) {
  // Timing stats
  total_time += other->total_time;

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
  total_forced_gvts += other->total_forced_gvts;
  mem_forced_gvts += other->mem_forced_gvts;
  end_forced_gvts += other->end_forced_gvts;
  event_forced_gvts += other->event_forced_gvts;
  fossil_collect_calls += other->fossil_collect_calls;

  // Memory stats
  max_memory_used = fmax(max_memory_used, other->max_memory_used);
  max_events_used = fmax(max_events_used, other->max_events_used);
  new_event_calls += other->new_event_calls;
  del_event_calls += other->del_event_calls;
}

void Statistics::reduce(const Statistics* other) {
  // Timing stats
  total_time = fmax(total_time, other->total_time);

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
  total_forced_gvts = other->total_forced_gvts;
  mem_forced_gvts = other->mem_forced_gvts;
  end_forced_gvts = other->end_forced_gvts;
  event_forced_gvts = other->event_forced_gvts;
  fossil_collect_calls += other->fossil_collect_calls;


  // Memory stats
  max_memory_used = fmax(max_memory_used, other->max_memory_used);
  max_events_used = fmax(max_events_used, other->max_events_used);
  new_event_calls += other->new_event_calls;
  del_event_calls += other->del_event_calls;
}

void Statistics::print_section(const char* name) const {
  CkPrintf("\n\n==== %-32s ==================================\n", name);
}

void Statistics::print_int(const char* name, tw_stat v) const {
  CkPrintf("\t%-50s %11lld\n", name, v);
}

void Statistics::print_double(const char* name, double v) const {
  CkPrintf("\t%-50s %11.2f\n", name, v);
}

void Statistics::print() const {
  // TODO: Add stats about PEs, num nodes, etc?
  print_section("SUMMARY");
  print_int("Events Committed", events_committed);
  print_double("Execution Time", total_time);
  print_double("Event Rate", events_committed / total_time);

  print_section("EVENT STATISTICS");
  print_int("Total Events Processed", events_executed);
  print_int("Events Committed", events_committed);
  print_int("Events Rolled Back", events_rolled_back);
  print_double("Efficiency", 100.0 * (1.0 - ((double) events_rolled_back / (double) events_committed)));

  print_section("COMMUNICATION STATISTICS");
  // TODO: Improve these stats to include remote % and also to track the sent
  // events that were committed vs those sent and cancelled
  print_int("Self Sends", self_sends);
  print_int("Local Sends", local_sends);
  print_int("Remote Sends", remote_sends);
  print_int("Anti Event Sends", anti_sends);

  print_section("ROLLBACK STATISTICS");
  // TODO: Cancellations as well?
  print_int("Total Rollback Calls ", total_rollback_calls);
  print_int("Timestamp Rollback Calls ", ts_rollback_calls);
  print_int("Event Rollback Calls ", event_rollback_calls);

  print_section("GVT STATISTICS");
  print_int("Total GVT Computations", total_gvts);
  print_int("Total Forced GVT Computations", total_forced_gvts);
  print_int("Memory Forced GVT Computations", mem_forced_gvts);
  print_int("End Time Forced GVT Computations", end_forced_gvts);
  print_int("Event Forced GVT Computations", event_forced_gvts);

  print_section("MEMORY STATISTICS");
  print_int("Max Memory Allocated Events on a PE (MB)", max_memory_used / (1024 * 1024));
  print_int("Max Events Allocated on a PE", max_events_used);
  print_int("Remote Event Allocations", new_event_calls);
  print_int("Remote Event Dealloactions", del_event_calls);

  print_section("MISC STATISTICS");
  print_int("Fossil Collect Calls", fossil_collect_calls);
}

#ifdef CMK_TRACE_ENABLED
void Statistics::init_tracing() {
    EVENTS_EXECUTED = traceRegisterUserStat("Events executed", -1);
    EVENTS_COMMITTED = traceRegisterUserStat("Events committed", -1);
    EVENTS_ROLLED_BACK = traceRegisterUserStat("Events rolled back", -1);

    SELF_SENDS = traceRegisterUserStat("Self sends", -1);
    LOCAL_SENDS = traceRegisterUserStat("Local sends", -1);
    REMOTE_SENDS = traceRegisterUserStat("Remote sends", -1);
    ANTI_SENDS = traceRegisterUserStat("Anti sends", -1);

    MAX_EVENTS_USED = traceRegisterUserStat("Max event used", -1);
    NEW_EVENT_CALLS = traceRegisterUserStat("New event calls", -1);
    DEL_EVENT_CALLS = traceRegisterUserStat("Delete event calls", -1);
}

void Statistics::log_tracing(int gvt_num) const {
  updateStatPair(EVENTS_EXECUTED, events_executed, gvt_num);
  updateStatPair(EVENTS_COMMITTED, events_committed, gvt_num);
  updateStatPair(EVENTS_ROLLED_BACK, events_rolled_back, gvt_num);

  updateStatPair(SELF_SENDS, self_sends, gvt_num);
  updateStatPair(LOCAL_SENDS, local_sends, gvt_num);
  updateStatPair(REMOTE_SENDS, remote_sends, gvt_num);
  updateStatPair(ANTI_SENDS, anti_sends, gvt_num);

  updateStatPair(MAX_EVENTS_USED, max_events_used, gvt_num);
  updateStatPair(NEW_EVENT_CALLS, new_event_calls, gvt_num);
  updateStatPair(DEL_EVENT_CALLS, del_event_calls, gvt_num);
}
#endif
