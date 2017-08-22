// TESTS TO ADD
// Multiple LPs per chare, which should still individually break ties per LP
// Different tie breaking per LP
// Type based tie breaking

#include "arithmetic.h"

int main(int argc, char** argv) {
  tw_init(argc, argv);

  register_msg_type<AddMessage>();
  register_msg_type<MultMessage>();

  ArithmeticFactory factory(get_msg_id<MultMessage>(), 32, 4);
  g_total_lps = factory.get_total_lps();
  g_tw_expected_events = factory.get_expected_events();

  g_tw_lookahead = 1;
  g_lps_per_chare = 1;

  tw_create_simulation(&factory);
  tw_run();
  tw_end();

  return 0;
}
