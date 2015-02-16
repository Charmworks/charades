#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "typedefs.h"

// A struct for holding PE level statistics
// An instance of this will be on each PE chare

struct Statistics {
    double s_max_run_time;
    double s_total_time;

    tw_stat s_net_events;
    tw_stat s_nevent_processed;
    tw_stat s_nevent_abort;
    tw_stat s_e_rbs;

    tw_stat s_rb_total;
    tw_stat s_rb_primary;
    tw_stat s_rb_secondary;
    tw_stat s_fc_attempts;

    tw_stat s_pq_qsize;
    tw_stat s_nsend_network;
    tw_stat s_nread_network;
    tw_stat s_nsend_remote_rb;

    tw_stat s_nsend_loc_remote;
    tw_stat s_nsend_net_remote;
    tw_stat s_ngvts;
    tw_stat s_mem_buffers_used;

    tw_stat s_pe_event_ties;

    tw_stime s_min_detected_offset;

    tw_clock s_total;
    tw_clock s_net_read;
    tw_clock s_gvt;
    tw_clock s_fossil_collect;

    tw_clock s_event_abort;
    tw_clock s_event_process;
    tw_clock s_pq;
    tw_clock s_rollback;

    tw_clock s_cancel_q;

    tw_clock s_avl;
};

// Function for setting initial values
inline void initialize_statistics(Statistics* statistics) {
  statistics->s_max_run_time = 0.0;
  statistics->s_net_events = 0;
  statistics->s_nevent_processed = 0;
  statistics->s_nevent_abort = 0;
  statistics->s_e_rbs = 0;
  statistics->s_rb_total = 0;
  statistics->s_rb_primary = 0;
  statistics->s_rb_secondary = 0;
  statistics->s_fc_attempts = 0;
  statistics->s_pq_qsize = 0;
  statistics->s_nsend_network = 0;
  statistics->s_nread_network = 0;
  statistics->s_nsend_remote_rb = 0;
  statistics->s_nsend_loc_remote = 0;
  statistics->s_nsend_net_remote = 0;
  statistics->s_ngvts = 0;
  statistics->s_mem_buffers_used = 0;
  statistics->s_pe_event_ties = 0;
  statistics->s_min_detected_offset = 0.0;
  statistics->s_total = 0;
  statistics->s_net_read = 0;
  statistics->s_gvt = 0;
  statistics->s_fossil_collect = 0;
  statistics->s_event_abort = 0;
  statistics->s_event_process = 0;
  statistics->s_pq = 0;
  statistics->s_rollback = 0;
  statistics->s_cancel_q = 0;
  statistics->s_avl = 0;
}

// Defined in pe.C
Statistics* get_statistics();

#define PE_STATS(x) get_statistics()->x

#endif
