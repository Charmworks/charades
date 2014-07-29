#include "ross_setup.h"
#include "../ross_opts/ross_opts.h"
#include "../globals.h"

#include <stdio.h>

#ifndef NO_GLOBALS
// TODO: Probably need a wrapper for initializing the chares
unsigned g_lps_per_chare = 16;
unsigned g_tw_synchronization_protocol;
tw_stime g_tw_ts_end;
unsigned g_tw_mblock;
unsigned g_tw_events_per_pe_extra;

unsigned g_tw_nlp;
size_t g_tw_memory_sz;
size_t g_tw_msg_sz;
tw_seed* g_tw_rng_seed;

FILE* g_tw_csv;

#endif

#ifndef NO_FORWARD_DECLS
const tw_optdef* tw_net_init(int* argc, char*** argv);
const tw_optdef* tw_gvt_setup();
// TODO: We may not use any clock stuff
const tw_optdef* tw_clock_setup();

int tw_ismaster();
void create_lps();
// TODO: ALl these net methods may be unnecessary with Charm++ as the backend
void tw_net_start();
void tw_gvt_start();

void tw_error(const char* file, int line, const char* fmt, ...);
#endif

#define PE_VALUE(x) get_globals()->x

// This can probably stay as a static global since it is only used at init for
// options. This is probably true of most options globals.
static const tw_optdef kernel_options[] = {
    TWOPT_GROUP("ROSS Kernel"),
    TWOPT_UINT("synch", PE_VALUE(g_tw_synchronization_protocol), "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4"),
// TODO: This will probably be replaced by number of chares
//    TWOPT_UINT("nkp", nkp_per_pe, "number of kernel processes (KPs) per pe"),
    TWOPT_STIME("end", PE_VALUE(g_tw_ts_end), "simulation end timestamp"),
    TWOPT_UINT("batch", PE_VALUE(g_tw_mblock), "messages per scheduler block"),
    TWOPT_UINT("extramem", PE_VALUE(g_tw_events_per_pe_extra), "Number of extra events allocated per PE."),
    TWOPT_END()
};

// TODO: This needs a lot of work
void tw_init(int* argc, char*** argv) {
  /** Add all of the command line options before parsing them **/
  tw_opt_add(tw_net_init(argc, argv));
  tw_opt_add(kernel_options);
  tw_opt_add(tw_gvt_setup());
  // TODO We may not use any clock stuff
  tw_opt_add(tw_clock_setup());

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

  tw_net_start();
  tw_gvt_start();
}

// TODO: In original ROSS this was defined in a processor centric way in that
// nlp, and g_tw_nlp were the number of lps on just this processor. Now we want
// to know the total number of LPs so we can create a chare array of that size.
void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed* seed) {
  // TODO: This should probably be in a group chare for config variables
  // TODO: What will this variable mean in the new ROSS? Right now it is lps on
  // on this PE which makes no sense. It should be total lps.
  PE_VALUE(g_tw_nlp) = nlp;

  // TODO: Nikhil is working on the memory management portion
#ifdef ROSS_MEMORY
  PE_VALUE(g_tw_memory_sz) = sizeof(tw_memory);
#endif

  PE_VALUE(g_tw_msg_sz) = msg_sz;
  PE_VALUE(g_tw_rng_seed) = seed;

  // TODO: Not implemented yet.
  /* early_sanity_check(); */

  // Only one processor should create the chare array
  if (tw_ismaster()) {
    // First we need to figure out the number of KPs (LP Chares)
    unsigned num_chares = nlp / PE_VALUE(g_lps_per_chare);

    // Create the lp chare array and store it in the readonly
    // TODO: We will eventually pass in a mapping function to the chare array
    // so it can properly determine which global ids it has.
    // The constructor also initializes the rng for each lp.
    // lps = CProxy_LP::ckNew(num_chares);
    create_lps();
  }
}
