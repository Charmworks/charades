#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "typedefs.h"

#include <float.h> // Included for DBL_MAX

// A struct for holding PE level statistics
// An instance of this will be on each PE chare

struct Statistics {
    // Timing stats
    double s_min_run_time; // Min run time among all PEs
    double s_max_run_time; // Max run time among all PEs

    // Event stats
    tw_stat s_nevent_processed; // Total number of events executed
    tw_stat s_net_events; // Number of events executed that weren't rolled back
    tw_stime s_min_detected_offset; // Minimum offset used in conservative mode

    // Rollback stats
    tw_stat s_e_rbs; // Total number of events rolled back
    tw_stat s_rb_total; // Total number of times a rollback is initiated
    tw_stat s_rb_primary; // Number of rollbacks to a certain timestamp
    tw_stat s_rb_secondary; // Number of rollbacks to a specific event

    // Send stats
    tw_stat s_nsend_loc_remote; // Events sent to different LPs but still the same chare
    tw_stat s_nsend_net_remote; // Events sent to different chares (may be on the same node)

    // GVT stats
    tw_stat s_ngvts; // Number of times we do a GVT calculation
    tw_stat s_fc_attempts; // Number of times we attempt to collect fossils
    tw_stat s_fossil_collect; // Number of times that there are actually 1 or more fossils to collect

    // Currently unused stats // TODO: Document or remove these
    tw_stat s_pq_qsize;
    tw_stat s_nsend_network;
    tw_stat s_nread_network;
    tw_stat s_nsend_remote_rb;
    tw_stat s_mem_buffers_used;
    tw_stat s_pe_event_ties;
    tw_clock s_total;
    tw_clock s_net_read;
    tw_clock s_gvt;
    tw_clock s_event_abort;
    tw_clock s_event_process;
    tw_clock s_pq;
    tw_clock s_rollback;
    tw_clock s_cancel_q;
    tw_clock s_avl;
    tw_stat s_nevent_abort;
};

// Function for setting initial values
inline void initialize_statistics(Statistics* statistics) {
  statistics->s_min_run_time = DBL_MAX;
  statistics->s_max_run_time = 0.0;

  statistics->s_nevent_processed = 0;
  statistics->s_net_events = 0;
  statistics->s_min_detected_offset = DBL_MAX;

  statistics->s_e_rbs = 0;
  statistics->s_rb_total = 0;
  statistics->s_rb_primary = 0;
  statistics->s_rb_secondary = 0;

  statistics->s_nsend_loc_remote = 0;
  statistics->s_nsend_net_remote = 0;

  statistics->s_ngvts = 0;
  statistics->s_fc_attempts = 0;
  statistics->s_fossil_collect = 0;

  statistics->s_pq_qsize = 0;
  statistics->s_nsend_network = 0;
  statistics->s_nread_network = 0;
  statistics->s_nsend_remote_rb = 0;
  statistics->s_mem_buffers_used = 0;
  statistics->s_pe_event_ties = 0;
  statistics->s_total = 0;
  statistics->s_net_read = 0;
  statistics->s_gvt = 0;
  statistics->s_event_abort = 0;
  statistics->s_event_process = 0;
  statistics->s_pq = 0;
  statistics->s_rollback = 0;
  statistics->s_cancel_q = 0;
  statistics->s_avl = 0;
  statistics->s_nevent_abort = 0;
}

// Defined in pe.C
Statistics* get_statistics();

#define PE_STATS(x) get_statistics()->x

#endif
