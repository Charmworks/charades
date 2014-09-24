#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "typedef.h"

// A struct for holding PE level statistics
// An instance of this will be on each PE chare

struct Statistics {
    double s_max_run_time;

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

Statistics* get_statistics();

#define PE_STATS(x) get_statistics()->x

#endif
