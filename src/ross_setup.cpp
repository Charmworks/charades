#include "ross_setup.h"

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

void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed* seed) {
  // TODO: This should probably be in a group chare for config variables
  g_tw_nlp = nlp;

  // First we need to figure out the number of KPs (LP Chares)

  // Then we need to create the LP chare array, passing it a mapping so it knows
  // how to map LPs to itself
}
