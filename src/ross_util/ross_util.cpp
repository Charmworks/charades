#include "ross_util.h"
#include "ross_event.h"

#include "charm_functions.h"
#include "charm_api.h"
#include "globals.h"

// Included for va_start etc.
#include <stdarg.h>

/*inline void gvt_print(Time gvt) {
  if(gvt_print_interval == 1.0) {
    return;
  }

  if(percent_complete == 0.0) {
    percent_complete = gvt_print_interval;
    return;
  }

  printf("GVT #%d: simulation %d%% complete (", PE_VALUE(lastGVT), (int) fmin(100, floor(100 * (gvt/PE_VALUE(g_tw_ts_end)))));

  if (gvt == DBL_MAX) {
    printf("GVT = %s", "MAX");
  } else {
    printf("GVT = %.4f", gvt);
  }

  printf(").\n");

  percent_complete += gvt_print_interval;
}*/

// From tw-stats.c
void show_lld(const char *name, tw_stat v) {
  printf("\t%-50s %11lld\n", name, v);
  fprintf(PE_VALUE(g_tw_csv), "%lld,", v);
}

void show_2f(const char *name, double v) {
  printf("\t%-50s %11.2f %%\n", name, v);
  fprintf(PE_VALUE(g_tw_csv), "%.2f,", v);
}

void show_1f(const char *name, double v) {
  printf("\t%-50s %11.1f\n", name, v);
  fprintf(PE_VALUE(g_tw_csv), "%.2f,", v);
}

void show_4f(const char *name, double v) {
  printf("\t%-50s %11.4lf\n", name, v);
  fprintf(PE_VALUE(g_tw_csv), "%.4lf,", v);
}

void tw_stats_old(Statistics *s) {
  int  i;

  size_t m_alloc, m_waste;

  // if (0 == g_tw_sim_started)
  //   return;

  // tw_calloc_stats(&m_alloc, &m_waste);

  // We need to gather group statistics here

//  me.s_max_run_time = max reduction on PE_VALUE(total_time)
  /*
  s.s_nevent_abort += pe->stats.s_nevent_abort;
  s.s_pq_qsize += tw_pq_get_size(me->pq);

  s.s_nsend_net_remote += pe->stats.s_nsend_net_remote;
  s.s_nsend_loc_remote += pe->stats.s_nsend_loc_remote;

  s.s_nsend_network += pe->stats.s_nsend_network;
  s.s_nread_network += pe->stats.s_nread_network;
  s.s_nsend_remote_rb += pe->stats.s_nsend_remote_rb;

  s.s_total += pe->stats.s_total;
  s.s_net_read += pe->stats.s_net_read;
  s.s_gvt += pe->stats.s_gvt;
  s.s_fossil_collect += pe->stats.s_fossil_collect;
  s.s_event_abort += pe->stats.s_event_abort;
  s.s_event_process += pe->stats.s_event_process;
  s.s_pq += pe->stats.s_pq;
  s.s_rollback += pe->stats.s_rollback;
  s.s_cancel_q += pe->stats.s_cancel_q;
  s.s_pe_event_ties += pe->stats.s_pe_event_ties;
  s.s_min_detected_offset = PE_VALUE(g_tw_min_detected_offset);
  s.s_avl += pe->stats.s_avl;

  for(i = 0; i < PE_VALUE(g_tw_nkp); i++)
  {
    kp = tw_getkp(i);
    s.s_nevent_processed += kp->s_nevent_processed;
    s.s_e_rbs += kp->s_e_rbs;
    s.s_rb_total += kp->s_rb_total;
    s.s_rb_secondary += kp->s_rb_secondary;
  }

  for(i = 0; i < PE_VALUE(g_tw_nlp); i++)
  {
    lp = tw_getlp(i);
    if (lp->type->final)
    (*lp->type->final) (lp->cur_state, lp);
  }


  s.s_fc_attempts = PE_VALUE(g_tw_fossil_attempts);
  s.s_net_events = s.s_nevent_processed - s.s_e_rbs;
  s.s_rb_primary = s.s_rb_total - s.s_rb_secondary;

  s = *(tw_net_statistics(me, &s));

  // End group statistic

*/
  if (!tw_ismaster())
    return;

  // Local print from pe 0

#ifndef ROSS_DO_NOT_PRINT
  printf("\n\t: Running Time = %.4f seconds\n", s->s_max_run_time);
  fprintf(PE_VALUE(g_tw_csv), "%.4f,", s->s_max_run_time);

  printf("\nTW Library Statistics:\n");
  show_lld("Total Events Processed", s->s_nevent_processed);
  show_lld("Events Aborted (part of RBs)", s->s_nevent_abort);
  show_lld("Events Rolled Back", s->s_e_rbs);
  show_lld("Event Ties Detected in PE Queues", s->s_pe_event_ties);
        if(PE_VALUE(g_tw_synchronization_protocol) == CONSERVATIVE)
            printf("\t%-50s %11.9lf\n",
               "Minimum TS Offset Detected in Conservative Mode",
               (double) s->s_min_detected_offset);
  show_2f("Efficiency", 100.0 * (1.0 - ((double) s->s_e_rbs / (double) s->s_net_events)));
  show_lld("Total Remote (shared mem) Events Processed", s->s_nsend_loc_remote);

  show_2f(
    "Percent Remote Events",
    ( (double)s->s_nsend_loc_remote
    / (double)s->s_net_events)
    * 100.0
  );

  show_lld("Total Remote (network) Events Processed", s->s_nsend_net_remote);
  show_2f(
    "Percent Remote Events",
    ( (double)s->s_nsend_net_remote
    / (double)s->s_net_events)
    * 100.0
  );

  printf("\n");
  show_lld("Total Roll Backs ", s->s_rb_total);
  show_lld("Primary Roll Backs ", s->s_rb_primary);
  show_lld("Secondary Roll Backs ", s->s_rb_secondary);
  show_lld("Fossil Collect Attempts", s->s_fc_attempts);
  show_lld("Total GVT Computations", s->s_ngvts);

  printf("\n");
  show_lld("Net Events Processed", s->s_net_events);
  show_1f(
    "Event Rate (events/sec)",
    ((double)s->s_net_events / s->s_max_run_time)
  );

  printf("\nTW Memory Statistics:\n");
  show_lld("Events Allocated", PE_VALUE(g_tw_max_events_buffered) * tw_nnodes());
  show_lld("Memory Allocated", m_alloc / 1024);
  show_lld("Memory Wasted", m_waste / 1024);

  if (tw_nnodes() > 1) {
    printf("\n");
    printf("TW Network Statistics:\n");
    show_lld("Remote sends", s->s_nsend_network);
    show_lld("Remote recvs", s->s_nread_network);
  }

/*
  printf("\nTW Data Structure sizes in bytes (sizeof):\n");
  show_lld("PE struct", sizeof(tw_pe));
  show_lld("KP struct", sizeof(tw_kp));
  show_lld("LP struct", sizeof(tw_lp));
  show_lld("LP Model struct", lp->type->state_sz);
  show_lld("LP RNGs", sizeof(*lp->rng));
  show_lld("Total LP", sizeof(tw_lp) + lp->type->state_sz + sizeof(*lp->rng));
  show_lld("Event struct", sizeof(tw_event));
  show_lld("Event struct with Model", sizeof(tw_event) + PE_VALUE(g_tw_msg_sz));
*/

#ifdef ROSS_timing
  printf("\nTW Clock Cycle Statistics (MAX values in secs at %1.4lf GHz):\n", PE_VALUE(g_tw_clock_rate) / 1000000000.0);
  show_4f("Priority Queue (enq/deq)", (double) s->s_pq / PE_VALUE(g_tw_clock_rate));
    show_4f("AVL Tree (insert/delete)", (double) s->s_avl / PE_VALUE(g_tw_clock_rate));
  show_4f("Event Processing", (double) s->s_event_process / PE_VALUE(g_tw_clock_rate));
  show_4f("Event Cancel", (double) s->s_cancel_q / PE_VALUE(g_tw_clock_rate));
  show_4f("Event Abort", (double) s->s_event_abort / PE_VALUE(g_tw_clock_rate));
  printf("\n");
  show_4f("GVT", (double) s->s_gvt / PE_VALUE(g_tw_clock_rate));
  show_4f("Fossil Collect", (double) s->s_fossil_collect / PE_VALUE(g_tw_clock_rate));
  show_4f("Primary Rollbacks", (double) s->s_rollback / PE_VALUE(g_tw_clock_rate));
  show_4f("Network Read", (double) s->s_net_read / PE_VALUE(g_tw_clock_rate));
  show_4f("Total Time (Note: Using Running Time above for Speedup)", (double) s->s_total / PE_VALUE(g_tw_clock_rate));
#endif

  //tw_gvt_stats(stdout);
#endif

}


/**
 * Rollback-aware printf, i.e. if the event gets rolled back, undo the printf.
 * We can'd do that of course so we store the message in a buffer until GVT.
 */
int tw_output(tw_lp *lp, const char *fmt, ...) {
  int ret = 0;
  va_list ap;
  tw_event *cev;
  tw_out *temp;

  if (PE_VALUE(g_tw_synchronization_protocol) != OPTIMISTIC) {
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    return 0;
  }

  tw_out *out = allocate_output_buffer();

  cev = current_event(lp);

  if (cev->out_msgs == NULL) {
    cev->out_msgs = out;
  } else {
    temp = cev->out_msgs;

    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = out;
  }

  va_start(ap, fmt);
  ret = vsnprintf(out->message, sizeof(out->message), fmt, ap);
  va_end(ap);
  if (ret >= 0 && ret < sizeof(out->message)) {
    // Should be successful
  } else {
    tw_printf(TW_LOC, "Message may be too large?");
  }

  return ret;
}

void tw_printf(const char *file, int line, const char *fmt, ...) {
  va_list	ap;

  va_start(ap, fmt);
  fprintf(stdout, "%s:%i: ", file, line);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);
  va_end(ap);
}

void tw_error(const char *file, int line, const char *fmt, ...) {
  va_list	ap;

  va_start(ap, fmt);
  // TODO: C API for CkMyPE?
  fprintf(stdout, "node: %d: error: %s:%i: ", tw_mype(), file, line);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);
  fflush(stdout);
  va_end(ap);

  tw_abort("Abort called\n");
}
