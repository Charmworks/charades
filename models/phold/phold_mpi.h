#include "ross.h"

#undef ROSS_CONSTANT
#define ROSS_CONSTANT(x) x

tw_stime lookahead = 0.1;
static unsigned int offset_lpid = 0;
static unsigned int g_total_lps = 0;
static tw_stime mult = 1.4;
static unsigned int nlp_per_pe = 8;
static int optimistic_memory = 100;

#include "phold_common.h"

tw_peid
phold_map(tw_lpid gid)
{
  return (tw_peid) gid / g_tw_nlp;
}

tw_lptype       mylps[] = {
  {(init_f) phold_init,
     /* (pre_run_f) phold_pre_run, */
     (pre_run_f) NULL,
   (event_f) phold_event_handler,
   (revent_f) phold_event_handler_rc,
   (final_f) phold_finish,
   (map_f) phold_map,
  sizeof(phold_state)},
  {0},
};

const tw_optdef mpi_opt[] =
{
  TWOPT_GROUP("PHOLD MPI Model"),
  TWOPT_STIME("lookahead", lookahead, "desired lookahead"),
  TWOPT_UINT("nlp", nlp_per_pe, "number of lps per pe"),
  TWOPT_END()
};
