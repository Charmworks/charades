/** \file statistics.h
 *  Declaration of the Statistics class and associated methods/variables.
 *
 *  Currently, each PE maintains a single instance of a Statistics object, and
 *  at the end of the simulation, a reduction is done to compute total
 *  statistics. Some of the PE level statistics may also be logged for
 *  projections intermittently if tracing is turned on, which requires an
 *  additional instance a Statistics object on each PE.
 *
 *  \todo More modular statistics
 *    - A top level composite statistics class
 *    - Smaller stats modules for things like scheduler, GVT, etc.
 *    - Modules register with the top level class
 *
 *  \todo LP level statistics/tracing
 *    - Related to above, could be a module, however there would be one per LP
 *    - Tracing would require some added functionality to Projections
 *
 *  \todo Better stats tracing
 *    - Probably subclass or composite pattern to make logging self-contained
 *    - Calling log() would dump current stats to file, and accumulate
 *    - Final reduction only needs aggregate stats
 *    - Maybe have a separate type for stats that is a pair when tracing is
 *      enabled, which will make all stats traceable more easily
 *
 *  \todo Model specific stats and logging
 *    - May be entirely taken care of by modular and lp level above
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "typedefs.h"

#include <charm++.h>

/** MACRO to more easily access PE local statistics */
#define PE_STATS(x) get_statistics()->x

/**
 * Class for holding a single stat, and computing its min, max, and sum (across
 * PEs) when doing the final stat reduction.
 */
template <typename T>
class Stat {
public:
  T total;    ///< Total (across PEs), also the current value during the sim
  T minimum;  ///< Maximum value across PEs
  T maximum;  ///< Minimum value across PEs

  Stat() { clear(); } ///< Calls clear()

  /** Clears stat to "0" */
  void clear() {
    total = 0;
    minimum = 0;
    maximum = 0;
  }
  /** Sets min and max to total (call right before the reduction) */
  void finalize() {
    minimum = maximum = total;
  }
  /** Adds the totals */
  void add(const Stat<T>& other) {
    total += other.total;
  }
  /** Adds totals, mins the minumum, and maxes the maximum */
  void reduce(const Stat<T>& other) {
    total += other.total;
    minimum = std::min(minimum, other.minimum);
    maximum = std::max(maximum, other.maximum);
  }
  /** Adds the passed in value to t */
  void operator+=(const T& t) { total += t; }
};

/**
 * Class for holding all PE level statistics for a given simulation run.
 *
 * Stats are incremented within the simulation code as needed. If stats tracing
 * is turned on, then one instance of the Statistics class holds data for the
 * most recent interval, and another instance holds accumulated stats for the
 * entire run. Stats need to be aggregatable (for tracing) and reducible (for
 * end of run reduction).
 */
class Statistics {
  public:
    /** \name Timing stats *////@{
    Stat<double> total_time;    ///< Total execution time
#ifdef DETAILED_TIMING
    Stat<double> execute_time;  ///< Time in Scheduler::execute (just events)
    Stat<double> fossil_time;   ///< Time in fossil collection
    Stat<double> gvt_time;      ///< Time from starting GVT to gvt_done()
    Stat<double> gvt_delay;     ///< Time blocking on the GVT computation
    Stat<double> lb_time;       ///< Time spent load balancing
#endif
    ///@}

    /** \name Event stats *////@{
    uint64_t events_executed;    ///< total number of events executed
    uint64_t events_committed;   ///< number of committed events
    uint64_t events_rolled_back; ///< number of rolled back events
    ///@}

    /** \name Rollback stats *////@{
    uint64_t total_rollback_calls; ///< total number of times a rollback is called
    uint64_t ts_rollback_calls;    ///< number of rollbacks to a certain timestamp
    uint64_t event_rollback_calls; ///< number of rollbacks to a specific event
    ///@}

    /** \name Send stats *////@{
    uint64_t self_sends;   ///< events sent to ourselves
    uint64_t local_sends;  ///< events sent to different LPs in the same chare
    uint64_t remote_sends; ///< events sent to different chares
    uint64_t remote_holds;
    uint64_t remote_cancels;
    uint64_t anti_sends;   ///< anti events sent
    uint64_t anti_cancels;
    ///@}

    /** \name GVT stats
     *  \todo Move these to the GVT controllers and add more
     *////@{
    uint32_t total_gvts; ///< total number of GVT calculations
    ///@}

    /** \name LB stats
     *  \todo Can we get more from LB framework? ie max/avg rations, num migs
     *////@{
    uint32_t total_lbs;
    ///@}

    /** \name Memory stats *////@{
    uint32_t max_events_used;	///< max number of events allocated from buffer
    uint32_t new_event_calls;  ///< number of events allocated for an empty buffer
    uint32_t del_event_calls;  ///< number of events deleted for a full buffer
    ///@}

  /** Calls clear() to 0 out stats on creation */
  Statistics();

  /** Sets all stats to their "0" values */
  void clear();
  /** Finalizes stats (primarily used to set min/max in Stat objects) */
  void finalize();
  /** Adds the passed in statistics object to this one */
  void add(const Statistics*);
  /** Reduces the passed in statistics object with this one */
  void reduce(const Statistics*);

  /** \name Print Utility functions
   *  Utility functions for nicely printing out stats data
   *////@{
  void print_section(const char*) const;
  void print_int(const char*, uint64_t) const;
  void print_double(const char*, double) const;
  void print_stat_double(const char*, const Stat<double>&) const;
  void print() const;
  ///@}

#if CMK_TRACE_ENABLED
  /** \name Tracing Tags
   *  Enums for tracing each associated stat value used by projections
   *////@{
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
  ///@}
#endif
};

/** Helper function for accessing the local Statistics object */
Statistics* get_statistics();
/** Reducer function used by the Charm++ runtime when doing a stats reduction */
CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);
/** Called during node init to register the stats reduction type */
void registerStatsReduction();
/** The type of the stats reduction used by the Charm++ runtime */
extern CkReduction::reducerType statsReductionType;

#endif
