#include "ross_setup.h"

#include "ross_block.h"
#include "ross_opts.h"
#include "ross_util.h"

#include "charm_functions.h"
#include "charm_api.h"
#include "globals.h"
#include "avl_tree.h"

void tw_event_setup() {
  AvlTree avl_list = (AvlTree)calloc(sizeof(struct avlNode), AVL_NODE_COUNT);
  for (int i = 0; i < AVL_NODE_COUNT - 1; i++) {
    avl_list[i].next = &avl_list[i + 1];
  }
  avl_list[AVL_NODE_COUNT - 1].next = NULL;
  PE_VALUE(avl_list_head) = &avl_list[0];

  DEBUG_MASTER("Created AVL tree with %d nodes\n", AVL_NODE_COUNT);

  tw_out *output_head = (tw_out *)calloc(sizeof(struct tw_out), NUM_OUT_MESG);
  for (int i = 0; i < NUM_OUT_MESG - 1; i++) {
    output_head[i].next = &output_head[i + 1];
  }
  output_head[NUM_OUT_MESG - 1].next = NULL;
  PE_VALUE(output) = output_head;

  DEBUG_MASTER("Created %d output messages\n", NUM_OUT_MESG);

  PE_VALUE(event_buffer) = new EventBuffer(PE_VALUE(g_tw_max_events_buffered),
                                           PE_VALUE(g_tw_msg_sz));
  PE_VALUE(abort_event) = PE_VALUE(event_buffer)->get_abort_event();
  PE_VALUE(abort_event)->state.owner = TW_event_inf;

  DEBUG_MASTER("Created event buffer with %d events of size %d\n",
      PE_VALUE(g_tw_max_events_buffered), PE_VALUE(g_tw_msg_sz));
}

char **CopyArgs(char **argv)
{
  int argc=CmiGetArgc(argv);
  char **ret=(char **)malloc(sizeof(char *)*(argc+1));
  int i;
  for (i=0;i<=argc;i++)
    ret[i]=argv[i];
  return ret;
}


void tw_init(int* argc, char*** argv) {
  char **charmArg = CopyArgs(*argv);
  charm_init(*argc, charmArg);

  /** Add all of the command line options before parsing them **/
  static const tw_optdef kernel_options[] = {
    // TODO: Make sure all relevant constants can be set from the command line
    TWOPT_GROUP("ROSS Kernel"),
    TWOPT_UINT("synch", PE_VALUE(g_tw_synchronization_protocol), "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4"),
    TWOPT_STIME("end", PE_VALUE(g_tw_ts_end), "Simulation end timestamp"),
    TWOPT_STIME("lookahead", PE_VALUE(g_tw_lookahead), "Lookahead for events"),
    TWOPT_UINT("batch", PE_VALUE(g_tw_mblock), "Messages per scheduler block"),
    TWOPT_UINT("gvt-interval", PE_VALUE(g_tw_gvt_interval), "GVT Interval"),
    TWOPT_UINT("num-lps", PE_VALUE(g_total_lps), "Number of total LPs"),
    TWOPT_UINT("num-chares", PE_VALUE(g_num_chares), "Number of chares"),
    TWOPT_UINT("lps-per-chare", PE_VALUE(g_lps_per_chare), "LPs per chare"),
    TWOPT_UINT("buffer-size", PE_VALUE(g_tw_max_events_buffered), "Number of events buffered"),
    TWOPT_END()
  };

  tw_opt_add(kernel_options);

  // Print out command line, version, and time.
  if (tw_ismaster()) {
    for (int i = 0; i < *argc; i++) {
      printf("%s ", (*argv)[i]);
    }
    printf("\n\n");

#ifdef ROSS_VERSION
#if HAVE_CTIME
    time_t raw_time;
    time(&raw_time);
    printf("%s\n", ctime(&raw_time));
#endif
    printf("ROSS Revision: %s\n\n", ROSS_VERSION);
#endif
  }

  tw_opt_parse(argc, argv);

  if (tw_ismaster() && NULL == (PE_VALUE(g_tw_csv) = fopen("ross.csv", "a"))) {
    tw_error(TW_LOC, "Unable to open: ross.csv\n");
  }

  tw_opt_print();
  /** Set up all the buffers for events */
}

void tw_define_lps(size_t msg_sz, tw_seed* seed) {
  PE_VALUE(g_tw_msg_sz) = msg_sz;
  PE_VALUE(g_tw_rng_seed) = seed;

  tw_event_setup();

  // TODO: Not implemented yet.
  /* early_sanity_check(); */

  bool constant_per_chare = false;
  // Check consistency and calculate values for number of lps and chares
  if (PE_VALUE(g_numlp_map) == NULL) {
    PE_VALUE(g_numlp_map) = constant_numlp_map;
    constant_per_chare = true;

    if (PE_VALUE(g_total_lps) == 1) {
      PE_VALUE(g_total_lps) = PE_VALUE(g_num_chares) * PE_VALUE(g_lps_per_chare);
    } else if (PE_VALUE(g_num_chares) == 1) {
      PE_VALUE(g_num_chares) = PE_VALUE(g_total_lps) / PE_VALUE(g_lps_per_chare);
    } else if (PE_VALUE(g_lps_per_chare) == 1) {
      PE_VALUE(g_lps_per_chare) = PE_VALUE(g_total_lps) / PE_VALUE(g_num_chares);
    }

    // Check for consitency
    if (PE_VALUE(g_total_lps) != PE_VALUE(g_num_chares) * PE_VALUE(g_lps_per_chare)) {
      tw_error(TW_LOC, "Inconsistent values for g_total_lps, g_num_chares, and g_lps_per_chare\n");
    }
  } else {
    if (PE_VALUE(g_total_lps) == 1) {
      PE_VALUE(g_total_lps) = 0;
      for (int i = 0; i < PE_VALUE(g_num_chares); i++) {
        PE_VALUE(g_total_lps) += PE_VALUE(g_numlp_map)(i);
      }
    } else if (PE_VALUE(g_num_chares) == 1) {
      PE_VALUE(g_num_chares) = 0;
      unsigned sum = 0;
      while (sum < PE_VALUE(g_total_lps)) {
        sum += PE_VALUE(g_numlp_map)(PE_VALUE(g_num_chares)++);
      }
    }

    // Check for consistency
    unsigned sum = 0;
    for (int i = 0; i < PE_VALUE(g_num_chares); i++) {
      sum += PE_VALUE(g_numlp_map)(i);
    }
    if (sum != PE_VALUE(g_total_lps)) {
      tw_error(TW_LOC, "Inconsistent values for g_total_lps and g_num_chares when using a non-constant numlps per chare\n");
    }
    if (tw_ismaster()) {
      printf("Creating %d chares and a total of %d lps, with a variable number of lps per chare.\n",
          PE_VALUE(g_num_chares),
          PE_VALUE(g_total_lps));
    }
  }


  // Print ROSS configuration information
  if(tw_ismaster()) {
    printf("========================================\n");
    printf("ROSS Configuration.....................\n");
    printf("   Synch Protocol.........%u\n", PE_VALUE(g_tw_synchronization_protocol));
    printf("   End time...............%lf\n", PE_VALUE(g_tw_ts_end));
    printf("   Lookahead..............%lf\n", PE_VALUE(g_tw_lookahead));
    printf("   Batch Size.............%u\n", PE_VALUE(g_tw_mblock));
    printf("   GVT Interval...........%u\n", PE_VALUE(g_tw_gvt_interval));
    printf("   Total LPs..............%u\n", PE_VALUE(g_total_lps));
    printf("   Chares.................%u\n", PE_VALUE(g_num_chares));
    if (constant_per_chare) {
      printf("   LPs per Chare..........%u\n", PE_VALUE(g_lps_per_chare));
    } else {
      printf("   LPs per Chare..........variable\n");
    }
    printf("   Buffer size............%u\n", PE_VALUE(g_tw_max_events_buffered));
    printf("========================================\n\n");
  }

  // Create the lp chare array and store it in the readonly
  create_lps();
}

void tw_run() {
  init_lps();
  charm_run();
}

/* TODO: Check what this is meant to do and implement */
void tw_end() {
 charm_exit();
}
