#include "phold_charm.h"

int main(int argc, char **argv, char **env) {
  // Add the model specific options, then initialize ROSS and Charm++
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  // Check for a valid configuration
  if (ROSS_CONSTANT(g_tw_lookahead) > 1.0) {
    tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
  }
  if (long_start_events > start_events) {
    tw_error(TW_LOC, "You specified more long start events than total start events\n");
  }

  // Adjust means based on lookahead
  mean = mean - ROSS_CONSTANT(g_tw_lookahead);
  long_mean = long_mean - ROSS_CONSTANT(g_tw_lookahead);

  // Type map must be set before tw_define_lps, all other maps will be default
  ROSS_CONSTANT(g_type_map) = phold_type_map;

  // Call tw_define_lps to create LPs and event queues
  tw_define_lps(sizeof(phold_message), 0);

  if (tw_ismaster()) {
    printf("========================================\n");
    printf("PHOLD Model Configuration..............\n");
    printf("   Start-events...........%u\n", start_events);
    printf("   Start-events (long)....%u\n", long_start_events);
    printf("   stagger................%u\n", stagger);
    printf("   Mean...................%lf\n", mean);
    printf("   Mean (long)............%lf\n", long_mean);
    printf("   Remote.................%lf\n", percent_remote);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();

  return 0;
}
