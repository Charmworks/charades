#include "setup.h"

#include "command_line.h"
#include "factory.h"
#include "mapper.h"
#include "scheduler.h"
#include "statistics.h"

CProxy_Initialize mainProxy;

Initialize::Initialize(CkArgMsg *m) {
  delete m;
  mainProxy = thisProxy;
  CkExit();
}

void tw_init(int argc, char** argv) {
  clear_globals();
  charm_init(argc, argv);
  parse_command_line(CmiGetArgc(argv), argv);
}

void tw_create_simulation(LPFactory* factory) {
  tw_create_simulation(factory, new BlockMapper());
}

void tw_create_simulation(LPFactory* factory, LPMapper* mapper) {
  g_lp_factory = factory;
  g_lp_mapper = mapper;
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
    g_lp_mapper = new BlockMapper();
  } else {
    if (g_total_lps == 1) {
      g_total_lps = 0;
      for (int i = 0; i < g_num_chares; i++) {
        g_total_lps += g_lp_mapper->get_num_lps(i);
      }
    } else if (g_num_chares == 1) {
      g_num_chares = 0;
      uint64_t sum = 0;
      while (sum < g_total_lps) {
        sum += g_lp_mapper->get_num_lps(g_num_chares++);
      }
    }
  }

  // TODO Update this
  // Print out configuration info and create the chares
  if (tw_ismaster()) {
    printf("========================================\n");
    printf("Charades Configuration..................\n");
    printf("   Synch Protocol.........%u\n", g_tw_synchronization_protocol);
    printf("   End time...............%llu\n", g_tw_ts_end);
    printf("   Lookahead..............%llu\n", g_tw_lookahead);
    printf("   Batch Size.............%u\n", g_tw_mblock);
    printf("   GVT Interval...........%u\n", g_tw_gvt_interval);
    printf("   Total LPs..............%u\n", g_total_lps);
    printf("   Chares.................%u\n", g_num_chares);
    printf("   Buffer size............%u\n", g_event_buffer_size);
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

void charm_init(int argc, char** argv) {
  CharmInit(argc, argv);
  traceRegisterUserEvent("Forward Execution", USER_EVENT_FWD);
  traceRegisterUserEvent("Rollback", USER_EVENT_RB);
  traceRegisterUserEvent("Cancellation", USER_EVENT_CANCEL);
  traceRegisterUserEvent("GVT", USER_EVENT_GVT);
  traceRegisterUserEvent("LB", USER_EVENT_LDB);
  traceRegisterUserEvent("Fossil Collection", USER_EVENT_FC);
}
void charm_exit() {
  CharmLibExit();
}
void charm_run() {
  ((Scheduler*)CkLocalBranch(scheduler_id))->start_simulation();
  StartCharmScheduler();
}

int tw_ismaster() {
  return (CkMyPe() == 0);
}
int tw_nnodes() {
  return CkNumPes();
}
int tw_mype() {
  return CkMyPe();
}

#include "setup.def.h"
