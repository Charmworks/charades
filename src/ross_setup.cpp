#include "ross_setup.h"

// Readonly array of lp chares.
extern CProxy_LP lps;

// TODO: This needs a lot of work
void tw_init(int* argc, char*** argv) {
  /** Add all of the command line options before parsing them **/
  /*tw_opt_add(tw_net_init(argc, argv));
  tw_opt_add(kernel_options);
  tw_opt_add(tw_gvt_setup());
  tw_opt_add(tw_clock_setup());*/

  tw_opt_parse(argc, argv);
  tw_opt_print();
}

// TODO: In original ROSS this was defined in a processor centric way in that
// nlp, and g_tw_nlp were the number of lps on just this processor. Now we want
// to know the total number of LPs so we can create a chare array of that size.
void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed* seed) {
  // TODO: This should probably be in a group chare for config variables
  // TODO: What will this variable mean in the new ROSS? Right now it is lps on
  // on this PE which makes no sense. It should be total lps.
  g_tw_nlp = nlp;

  // TODO: Nikhil is working on the memory management portion
#ifdef ROSS_MEMORY
  g_tw_memory_sz = sizeof(tw_memory);
#endif

  g_tw_msg_sz = msg_sz;
  g_tw_rng_seed = seed;

  // TODO: Not implemented yet.
  /* early_sanity_check(); */

  // Only one processor should create the chare array
  if (tw_ismaster()) {
    // First we need to figure out the number of KPs (LP Chares)
    unsigned num_chares = nlp / g_lps_per_chare;

    // Create the lp chare array and store it in the readonly
    // TODO: We will eventually pass in a mapping function to the chare array
    // so it can properly determine which global ids it has.
    // The constructor also initializes the rng for each lp.
    lps = CProxy_LP::ckNew(num_chares);
  }
}
