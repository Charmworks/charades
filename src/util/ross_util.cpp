#include "ross_util.h"
#include "event.h"

#include "charm_functions.h"
#include "globals.h"

// Included for va_start etc.
#include <stdarg.h>

// From tw-stats.c
/*void show_lld(const char *name, tw_stat v) {
  printf("\t%-50s %11lld\n", name, v);
  fprintf(PE_VALUE(g_tw_csv), "%lld,", v);
}

void show_2f(const char *name, double v) {
  printf("\t%-50s %11.2f %%\n", name, v);
  fprintf(PE_VALUE(g_tw_csv), "%.2f,", v);
}*/

void show_1f(const char *name, double v) {
  printf("\t%-50s %11.1f\n", name, v);
  //fprintf(PE_VALUE(g_tw_csv), "%.2f,", v);
}

void show_4f(const char *name, double v) {
  printf("\t%-50s %11.4lf\n", name, v);
  //fprintf(PE_VALUE(g_tw_csv), "%.4lf,", v);
}

/**
 * Prints out final stats for a simulation.
 */
void tw_stats(Statistics *s) {
#if 0
  size_t m_alloc, m_waste;

  // Calculate the net events based on total event executed and rolled back
  s->s_net_events = s->s_nevent_processed - s->s_e_rbs;
  
#ifndef ROSS_DO_NOT_PRINT
  printf("\n\t: Max PE run time = %.4f seconds\n", PE_STATS(s_max_run_time));
  printf("\n\t: Min PE run time = %.4f seconds\n", PE_STATS(s_min_run_time));
  fprintf(PE_VALUE(g_tw_csv), "%.4f,", PE_STATS(s_max_run_time));

  printf("\nTW Library Statistics:\n");
  show_lld("Total Events Processed", s->s_nevent_processed);
  //show_lld("Commited Events", s->s_committed_events);
  show_lld("Events Aborted (part of RBs)", s->s_nevent_abort);
  show_lld("Events Rolled Back", s->s_e_rbs);
  show_lld("Event Ties Detected in PE Queues", s->s_pe_event_ties);

  if (g_tw_synchronization_protocol == CONSERVATIVE) {
    printf("\t%-50s %11.9lf\n",
        "Minimum TS Offset Detected in Conservative Mode",
        (double) s->s_min_detected_offset);
  }

  show_2f("Efficiency", 100.0 * (1.0 - ((double) s->s_e_rbs / (double) s->s_net_events)));

  // There are two categories of remote events: local are those that were events
  // for different LPs on the same chare (therefore the event wasn't handled by
  // the Charm++ RTS), and network are events sent to different chares through
  // the Charm++ RTS.
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

  // Rollback and GVT stats
  show_lld("Total Roll Backs ", s->s_rb_total);
  show_lld("Primary Roll Backs ", s->s_rb_primary);
  show_lld("Secondary Roll Backs ", s->s_rb_secondary);
  show_lld("Fossil Collect Attempts", s->s_fc_attempts);
  show_lld("Successful Fossil Attempts", s->s_fossil_collect);
  show_lld("Total GVT Computations", s->s_ngvts);
  show_lld("Total Forced GVT Computations", s->s_forced_gvts);
  show_lld("Memory Forced GVT Computations", s->s_forced_mem_gvts);
  show_lld("End Time Forced GVT Computations", s->s_forced_end_gvts);
  show_lld("Event Forced GVT Computations", s->s_forced_event_gvts);
  printf("\n");
  //Memory Stats
  show_lld("Max Allocated Events per PE", s->s_max_allocated);
  show_1f("Average Max Allocated Events per PE", ( (double) s->s_avg_max_allocated)/((double)CkNumPes()));
  show_1f("Extra Remote Events Allocated per PE",((double)s->s_remote_new_allocated)/((double)CkNumPes()));
  show_1f("Remote Events Deleted per PE", ((double)s->s_remote_deallocated)/((double)CkNumPes())); 

  show_1f("Max Memory Usage in a PE (MB)", ((double) s->s_max_memory / (1024. * 1024)));

  // Summary of events processed and event rate
  show_lld("Net Events Processed", s->s_net_events);
  show_1f(
    "Event Rate (events/sec)",
    ((double)s->s_net_events / PE_STATS(s_max_run_time)));*/

  // TODO: Everything below here is unused...either implement or remove
  // TODO: Check that memory usage is correct
  /*printf("\nTW Memory Statistics:\n");
  show_lld("Events Allocated", g_tw_max_events_buffered);
  show_lld("Memory Allocated", m_alloc / 1024);
  show_lld("Memory Wasted", m_waste / 1024);

  // TODO: What are these used for?
  if (tw_nnodes() > 1) {
    printf("\n");
    printf("TW Network Statistics:\n");
    show_lld("Remote sends", s->s_nsend_network);
    show_lld("Remote recvs", s->s_nread_network);
  }

  printf("\nTW Data Structure sizes in bytes (sizeof):\n");
  show_lld("PE struct", sizeof(tw_pe));
  show_lld("KP struct", sizeof(tw_kp));
  show_lld("LP struct", sizeof(tw_lp));
  show_lld("LP Model struct", lp->type->state_sz);
  show_lld("LP RNGs", sizeof(*lp->rng));
  show_lld("Total LP", sizeof(tw_lp) + lp->type->state_sz + sizeof(*lp->rng));
  show_lld("Event struct", sizeof(tw_event));
  show_lld("Event struct with Model", sizeof(tw_event) + g_tw_msg_sz);

#ifdef ROSS_timing
  printf("\nTW Clock Cycle Statistics (MAX values in secs at %1.4lf GHz):\n", g_tw_clock_rate / 1000000000.0);
  show_4f("Priority Queue (enq/deq)", (double) s->s_pq / g_tw_clock_rate);
    show_4f("AVL Tree (insert/delete)", (double) s->s_avl / g_tw_clock_rate);
  show_4f("Event Processing", (double) s->s_event_process / g_tw_clock_rate);
  show_4f("Event Cancel", (double) s->s_cancel_q / g_tw_clock_rate);
  show_4f("Event Abort", (double) s->s_event_abort / g_tw_clock_rate);
  printf("\n");
  show_4f("GVT", (double) s->s_gvt / g_tw_clock_rate);
  show_4f("Fossil Collect", (double) s->s_fossil_collect / g_tw_clock_rate);
  show_4f("Primary Rollbacks", (double) s->s_rollback / g_tw_clock_rate);
  show_4f("Network Read", (double) s->s_net_read / g_tw_clock_rate);
  show_4f("Total Time (Note: Using Running Time above for Speedup)", (double) s->s_total / g_tw_clock_rate);
#endif

  //tw_gvt_stats(stdout);*/
#endif

  if (g_tw_expected_events && g_tw_expected_events != s->s_net_events) {
    tw_error(TW_LOC,
             "TEST RESULT: FAILURE!\nNet Events (%i) != Expected Events (%i)\n",
             s->s_net_events, g_tw_expected_events);
  }
#endif
}


/**
 * Rollback-aware printf, i.e. if the event gets rolled back, undo the printf.
 * We can'd do that of course so we store the message in a buffer until GVT.
 */
int tw_output(tw_lp *lp, const char *fmt, ...) {
  int ret = 0;
#if 0
  va_list ap;
  tw_event *cev;
  tw_out *temp;

  if (g_tw_synchronization_protocol != OPTIMISTIC) {
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
#endif

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
