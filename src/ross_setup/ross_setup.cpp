#include "ross_setup.h"
#include "ross_opts.h"
#include "ross_util.h"

#include "charm_functions.h"
#include "globals.h"
#include "avl_tree.h"

void tw_event_setup() {
  AvlTree avl_list = (AvlTree)calloc(sizeof(struct avlNode), AVL_NODE_COUNT);
  for (int i = 0; i < AVL_NODE_COUNT - 1; i++) {
    avl_list[i].next = &avl_list[i + 1];
  }
  avl_list[AVL_NODE_COUNT - 1].next = NULL;
  PE_VALUE(avl_list_head) = &avl_list[0];

  tw_out *output_head = (tw_out *)calloc(sizeof(struct tw_out), NUM_OUT_MESG);
  for (int i = 0; i < NUM_OUT_MESG - 1; i++) {
    output_head[i].next = &output_head[i + 1];
  }
  output_head[NUM_OUT_MESG - 1].next = NULL;
  PE_VALUE(output) = output_head;

  PE_VALUE(abort_event) = allocateEvent(0);
  PE_VALUE(abort_event)->state.owner = TW_event_inf;
}

void tw_init(int* argc, char*** argv) {
  charm_init(*argc, *argv);
  // TODO (eric): After the charm_lib_init() returns we need to copy user
  // options over to the PE global variables.
  /** Add all of the command line options before parsing them **/
  if(tw_ismaster()) DEBUG("Finished charm_init\n");
  static const tw_optdef kernel_options[] = {
    TWOPT_GROUP("ROSS Kernel"),
    TWOPT_UINT("synch", PE_VALUE(g_tw_synchronization_protocol), "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4"),
// TODO: This will probably be replaced by number of chares
//    TWOPT_UINT("nkp", nkp_per_pe, "number of kernel processes (KPs) per pe"),
    TWOPT_STIME("end", PE_VALUE(g_tw_ts_end), "simulation end timestamp"),
    TWOPT_UINT("batch", PE_VALUE(g_tw_mblock), "messages per scheduler block"),
    TWOPT_UINT("extramem", PE_VALUE(g_tw_events_per_pe_extra), "Number of extra events allocated per PE."),
    TWOPT_UINT("gvt-interval", PE_VALUE(g_tw_gvt_interval), "GVT Interval"),
    TWOPT_UINT("lps-per-chare", PE_VALUE(g_lps_per_chare), "LPs per chare"),
    TWOPT_UINT("num-chares", PE_VALUE(g_num_lp_chares), "Number of chares"),
    TWOPT_END()
  };

  tw_opt_add(kernel_options);
  if(tw_ismaster()) DEBUG("[%d] Added kernel options\n", CkMyPe());

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
  if(tw_ismaster()) DEBUG("[%d] Parsed opts\n", CkMyPe());

  if (tw_ismaster() && NULL == (PE_VALUE(g_tw_csv) = fopen("ross.csv", "a"))) {
    tw_error(TW_LOC, "Unable to open: ross.csv\n");
  }
  if(tw_ismaster()) DEBUG("[%d] Opened csv\n", CkMyPe());

  tw_opt_print();
  /** Set up all the buffers for events */
  if(tw_ismaster()) DEBUG("[%d] Printed opts\n", CkMyPe());
  tw_event_setup();
  if(tw_ismaster()) DEBUG("[%d] Event set up done\n", CkMyPe());
}

// TODO: In original ROSS this was defined in a processor centric way in that
// nlp, and g_tw_nlp were the number of lps on just this processor. Now we want
// to know the total number of LPs so we can create a chare array of that size.
void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed* seed) {
  // TODO: This should probably be in a group chare for config variables
  // TODO: What will this variable mean in the new ROSS? Right now it is lps on
  // on this PE which makes no sense. It should be total lps.
  PE_VALUE(g_tw_nlp) = nlp;
  PE_VALUE(g_tw_msg_sz) = msg_sz;
  PE_VALUE(g_tw_rng_seed) = seed;

  // TODO: Not implemented yet.
  /* early_sanity_check(); */

  // First we need to figure out the number of KPs (LP Chares)
  PE_VALUE(g_num_lp_chares) = (nlp * tw_nnodes()) / PE_VALUE(g_lps_per_chare);

  if(tw_ismaster()) DEBUG("[%d] Calling create lps\n", CkMyPe());
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
