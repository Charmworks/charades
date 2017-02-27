#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "typedefs.h"

#include <charm++.h>

// A struct for holding PE level statistics
// Two instances of this will be on each PE, one for stats for the current GVT
// period, and one for the accumulated stats for the whole run.

class Statistics {
  public:
    // Timing stats
    double total_time;  // Execution time these stats were collected during

    // Event stats
    tw_stat events_executed;    // Total number of events executed
    tw_stat events_committed;   // Number of committed events
    tw_stat events_rolled_back; // Number of rolled back events

    // Rollback stats
    tw_stat total_rollback_calls; // Total number of times a rollback is called
    tw_stat ts_rollback_calls;    // Number of rollbacks to a certain timestamp
    tw_stat event_rollback_calls; // Number of rollbacks to a specific event

    // Send stats
    tw_stat self_sends;   // Events sent to ourselves
    tw_stat local_sends;  // Events sent to different LPs in the same chare
    tw_stat remote_sends; // Events sent to different chares
    tw_stat anti_sends;   // Anti events sent

    // GVT stats
    // TODO: Move these to the standalone GVT controller once it exists
    tw_stat total_gvts;           // Total number of GVT calculations
    tw_stat total_forced_gvts;    // Number of times we force a GVT calculation
    tw_stat mem_forced_gvts;      // GVTs forced by low event memory
    tw_stat end_forced_gvts;      // GVTs forced by end time
    tw_stat event_forced_gvts;    // GVTs forced by empty event queues
    tw_stat fossil_collect_calls; // Number of fossil collection calls

    // Memory stats
    tw_stat max_memory_used;  // Max memory allocated at any GVT
    tw_stat max_events_used;	// Max number of events allocated from buffer
    tw_stat new_event_calls;  // Number of events allocated for an empty buffer
    tw_stat del_event_calls;  // Number of events deleted for a full buffer

  Statistics();

  void clear();
  void add(const Statistics*);
  void reduce(const Statistics*);

  void print_section(const char*) const;
  void print_int(const char*, tw_stat) const;
  void print_double(const char*, double) const;
  void print() const;

#ifdef CMK_TRACE_ENABLED
  int EVENTS_EXECUTED;
  int EVENTS_COMMITTED;
  int EVENTS_ROLLED_BACK;

  int EFFICIENCY;

  int SELF_SENDS;
  int LOCAL_SENDS;
  int REMOTE_SENDS;
  int ANTI_SENDS;

  int MAX_EVENTS_USED;
  int NEW_EVENT_CALLS;
  int DEL_EVENT_CALLS;

  void init_tracing();
  void log_tracing(int) const;
#endif
};

Statistics* get_statistics();
CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);
void registerStatsReduction();
extern CkReduction::reducerType statsReductionType;

#define PE_STATS(x) get_statistics()->x

#endif
