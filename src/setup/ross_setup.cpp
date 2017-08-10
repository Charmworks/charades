#include "ross_setup.h"

#include "charm_setup.h"
#include "globals.h"
#include "lp.h"
#include "mpi-interoperate.h"
#include "ross_block.h"
#include "ross_opts.h"

char** CopyArgs(int& argc, char** argv) {
  argc = CmiGetArgc(argv);
  char** ret = new char*[argc];
  for (int i = 0; i < argc; i++) ret[i]=argv[i];
  return ret;
}

void tw_init(int argc, char** tmp_argv) {
  clear_globals();
  charm_init(argc, tmp_argv);
  char** argv = CopyArgs(argc, tmp_argv);

  /** Add all of the command line options before parsing them **/
  static const tw_optdef kernel_options[] = {
    // TODO: Make sure all relevant constants can be set from the command line
    TWOPT_GROUP("ROSS Kernel"),
    TWOPT_UINT("synch", g_tw_synchronization_protocol, "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4"),
    TWOPT_UINT("gvt", g_tw_gvt_scheme, "GVT Algorithm: SYNC=1, CD=2, PHASED=3, BUCKETED=4"),
    TWOPT_UINT("expected-events", g_tw_expected_events, "Expected number of net events, tested at the end of a simulation"),
    TWOPT_STIME("end", g_tw_ts_end, "Simulation end timestamp"),
    TWOPT_STIME("lookahead", g_tw_lookahead, "Lookahead for events"),
    TWOPT_UINT("batch", g_tw_mblock, "Messages per scheduler block"),
    TWOPT_UINT("gvt-interval", g_tw_gvt_interval, "GVT Interval"),
    TWOPT_UINT("gvt-trigger", g_tw_gvt_trigger, "GVT Trigger: COUNT=1, LEASH=2"),
    TWOPT_UINT("gvt-phases", g_tw_gvt_phases, "Number of GVT Phases"),
    TWOPT_UINT("gvt-bucket-size", g_tw_gvt_bucket_size, "Size of each bucket for Bucketed GVT"),
    TWOPT_UINT("async-reduction", g_tw_async_reduction, "0=sync red, 1=async red"),
    TWOPT_UINT("ldb-interval", g_tw_ldb_interval, "Load Balancing Interval"),
    TWOPT_UINT("max-ldb", g_tw_max_ldb, "Max number of load balancing calls"),
    TWOPT_UINT("ldb-metric", g_tw_ldb_metric, "Metric for measuring LB load"),
    TWOPT_UINT("ldb-metric-ts-abs", g_tw_metric_ts_abs, "Use absolute timestamps for LP loads"),
    TWOPT_UINT("ldb-metric-invert", g_tw_metric_invert, "Invert load metrics"),
    TWOPT_UINT("stat-interval", g_tw_stat_interval, "Stat Interval"),
    TWOPT_UINT("num-lps", g_total_lps, "Number of total LPs"),
    TWOPT_UINT("num-chares", g_num_chares, "Number of chares"),
    TWOPT_UINT("lps-per-chare", g_lps_per_chare, "LPs per chare"),
    TWOPT_UINT("buffer-size", g_tw_max_events_buffered, "Number of events buffered"),
    TWOPT_UINT("msg-buffer-size", g_tw_max_remote_events_buffered, "Number of events buffered"),
    TWOPT_UINT("report-interval", gvt_print_interval, "percent of runtime to print GVT"),
    TWOPT_END()
  };

  tw_opt_add(kernel_options);
  tw_opt_parse(&argc, &argv);
  delete [] argv;
}

void tw_define_lps(size_t msg_sz, tw_seed* seed) {
  g_tw_msg_sz = msg_sz;
  g_tw_rng_seed = seed;

  // TODO: Not implemented yet.
  /* early_sanity_check(); */

  // In sequential mode there can only be one chare. Enforce that here.
  if (g_tw_synchronization_protocol == SEQUENTIAL) {
    if (g_num_chares > 1) {
      CkPrintf("WARNING: In sequential mode there can only be one chare!\n");
      CkPrintf("Overriding number of chares to be 1.\n");
    }
    if (g_lps_per_chare != g_total_lps) {
      CkPrintf("WARNING: In sequential mode all lps must be on one chare!\n");
      CkPrintf("Overriding lps per chare to be %d.\n", g_total_lps);
    }
    g_num_chares = 1;
    g_lps_per_chare = g_total_lps;
    CkPrintf("WARNING: In sequential mode all LPs must map to chare 0\n");
    CkPrintf("Overriding LP placement maps.\n");
    g_numlp_map  = NULL;
    g_init_map   = init_block_map;
    g_local_map  = local_block_map;
    g_chare_map  = chare_block_map;
  }

  bool constant_per_chare = false;
  // Check consistency and calculate values for number of lps and chares
  if (g_numlp_map == NULL) {
    g_numlp_map = constant_numlp_map;
    constant_per_chare = true;

    if (g_total_lps == 1) {
      g_total_lps = g_num_chares * g_lps_per_chare;
    } else if (g_num_chares == 1) {
      g_num_chares = g_total_lps / g_lps_per_chare;
    } else if (g_lps_per_chare == 1) {
      g_lps_per_chare = g_total_lps / g_num_chares;
    }

    // Check for consitency
    if (g_total_lps != g_num_chares * g_lps_per_chare) {
      CkAbort("Inconsistent values for g_total_lps, g_num_chares, and g_lps_per_chare\n");
    }
  } else {
    if (g_total_lps == 1) {
      g_total_lps = 0;
      for (int i = 0; i < g_num_chares; i++) {
        g_total_lps += g_numlp_map(i);
      }
    } else if (g_num_chares == 1) {
      g_num_chares = 0;
      unsigned sum = 0;
      while (sum < g_total_lps) {
        sum += g_numlp_map(g_num_chares++);
      }
    }

    // Check for consistency
    unsigned sum = 0;
    for (int i = 0; i < g_num_chares; i++) {
      sum += g_numlp_map(i);
    }
    if (sum != g_total_lps) {
      CkAbort("Inconsistent values for g_total_lps and g_num_chares when using a non-constant numlps per chare\n");
    }
    if (tw_ismaster()) {
      printf("Creating %d chares and a total of %d lps, with a variable number of lps per chare.\n",
          g_num_chares,
          g_total_lps);
    }
  }

  // Print out configuration info and create the chares
  if (tw_ismaster()) {
    printf("========================================\n");
    printf("ROSS Configuration.....................\n");
    printf("   Synch Protocol.........%u\n", g_tw_synchronization_protocol);
    printf("   End time...............%llu\n", g_tw_ts_end);
    printf("   Lookahead..............%llu\n", g_tw_lookahead);
    printf("   Batch Size.............%u\n", g_tw_mblock);
    printf("   GVT Interval...........%u\n", g_tw_gvt_interval);
    printf("   Total LPs..............%u\n", g_total_lps);
    printf("   Chares.................%u\n", g_num_chares);
    if (constant_per_chare) {
      printf("   LPs per Chare..........%u\n", g_lps_per_chare);
    } else {
      printf("   LPs per Chare..........variable\n");
    }
    printf("   Buffer size............%u\n", g_tw_max_events_buffered);
    printf("========================================\n\n");
    switch(g_tw_synchronization_protocol) {
      case 1:
        CProxy_SequentialScheduler::ckNew();
        break;
      case 2:
        CProxy_ConservativeScheduler::ckNew();
        break;
      case 3:
        CProxy_OptimisticScheduler::ckNew();
        break;
      default:
        CkAbort("Unknown synchronization protocol\n");
    }
  }
  StartCharmScheduler();
}

void tw_run() {
  init_lps();
  charm_run();
}

/* TODO: Check what this is meant to do and implement */
void tw_end() {
 charm_exit();
}
