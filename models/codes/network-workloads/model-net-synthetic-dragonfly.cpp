/*
 * Copyright (C) 2015 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

/*
* The test program generates some synthetic traffic patterns for the model-net network models.
* currently it only support the dragonfly network model uniform random and nearest neighbor traffic patterns.
*/

#include "codes/model-net.h"
#include "codes/lp-io.h"
#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"
#include "codes/net/dragonfly.h"

#define PAYLOAD_SZ 2048
#include "model-net-synthetic.h"

static int num_servers_per_rep = 0;
static int num_routers_per_grp = 0;
static int num_nodes_per_grp = 0;
static int num_groups = 0;
static int num_nodes = 0;

/*static char lp_io_dir[256] = {'\0'};
static lp_io_handle io_handle;
static unsigned int lp_io_use_suffix = 0;
static int do_lp_io = 0;
static int num_msgs = 20;
static tw_stime sampling_interval = 800000;
static tw_stime sampling_end_time = 1600000;*/

#if 0 // TRACING CODE NOT ENABLED
/* setup for the ROSS event tracing
 * can have a different function for  rbev_trace_f and ev_trace_f
 * but right now it is set to the same function for both
 */
void svr_event_collect(svr_msg *m, tw_lp *lp, char *buffer, int *collect_flag)
{
    (void)lp;
    (void)collect_flag;
    int type = (int) m->svr_event_type;
    memcpy(buffer, &type, sizeof(type));
}

/* can add in any model level data to be collected along with simulation engine data
 * in the ROSS instrumentation.  Will need to update the last field in 
 * svr_model_types[0] for the size of the data to save in each function call
 */
void svr_model_stat_collect(svr_state *s, tw_lp *lp, char *buffer)
{
    (void)s;
    (void)lp;
    (void)buffer;
    return;
}

st_model_types svr_model_types[] = {
    {(rbev_trace_f) svr_event_collect,
     sizeof(int),
     (ev_trace_f) svr_event_collect,
     sizeof(int),
     (model_stat_f) svr_model_stat_collect,
     0},
    {NULL, 0, NULL, 0, NULL, 0}
};

static const st_model_types  *svr_get_model_stat_types(void)
{
    return(&svr_model_types[0]);
}

void svr_register_model_types()
{
    st_model_type_register("server", svr_get_model_stat_types());
}
#endif

/* type of synthetic traffic */
enum TRAFFIC {
  UNIFORM = 1,          /* sends message to a randomly selected node */
  NEAREST_GROUP = 2,    /* sends message to the node connected to the neighboring router */
  NEAREST_NEIGHBOR = 3  /* sends message to the next node (potentially connected to the same router) */
};

/* Command line configuration options */
static int traffic = 1;
static double arrival_time = 1000.0;
static char conf_file_name[128] = {'\0'};
const tw_optdef app_opt [] = {
  TWOPT_GROUP("Model net synthetic traffic " ),
  TWOPT_UINT("traffic", traffic, "UNIFORM RANDOM=1, NEAREST NEIGHBOR=2 "),
  TWOPT_STIME("arrival_time", arrival_time, "INTER-ARRIVAL TIME"),
  TWOPT_CHAR("codes-config", conf_file_name, "name of codes configuration file"),
  //TWOPT_UINT("num_messages", num_msgs, "Number of messages to be generated per terminal "),
  //TWOPT_STIME("sampling-interval", sampling_interval, "the sampling interval "),
  //TWOPT_STIME("sampling-end-time", sampling_end_time, "sampling end time "),
  //TWOPT_CHAR("lp-io-dir", lp_io_dir, "Where to place io output (unspecified -> no output"),
  //TWOPT_UINT("lp-io-use-suffix", lp_io_use_suffix, "Whether to append uniq suffix to lp-io directory (default 0)"),
  TWOPT_END()
};

tw_stime get_mean_interval() {
  return arrival_time;
}

tw_lpid get_dest(tw_lp* lp) {
  char group_name[MAX_NAME_LENGTH];
  char lp_type_name[MAX_NAME_LENGTH];
  int group_index, lp_type_index, rep_id, offset;
  char anno[MAX_NAME_LENGTH];
  tw_lpid local_dest, global_dest;

  codes_mapping_get_lp_info(lp->gid, group_name, &group_index, lp_type_name, &lp_type_index, anno, &rep_id, &offset);
  int local_id = codes_mapping_get_lp_relative_id(lp->gid, 0, 0);
  // In the case of uniform random traffic, send to a random destination
  if (traffic == UNIFORM) {
    local_dest = tw_rand_integer(lp->rng, 0, num_nodes - 1);
  } else if (traffic == NEAREST_GROUP) {
    local_dest = (local_id + num_nodes_per_grp) % num_nodes;
  } else if (traffic == NEAREST_NEIGHBOR) {
    local_dest =  (local_id + 1) % num_nodes;
  } else {
    CkAbort("ERROR: Invalid value for traffic\n");
  }
  assert(local_dest < num_nodes);

  global_dest = codes_mapping_get_lpid_from_relative(
      local_dest, group_name, lp_type_name, NULL, 0);
  // If Destination is self, then generate new destination
  if (global_dest == lp->gid) {
    local_dest = (local_dest+1) % (num_nodes-1);
    global_dest = codes_mapping_get_lpid_from_relative(
        local_dest, group_name, lp_type_name, NULL, 0);
  }
  return global_dest;
}
tw_lpid get_dest_rc(tw_lp* lp) {
  if (traffic == UNIFORM) {
    tw_rand_reverse_unif(lp->rng);
  } 
}

int main(int argc, char** argv) {
  tw_opt_add(app_opt);
  tw_init(argc, argv);

  if (!conf_file_name[0]) {
      fprintf(stderr, "Expected \"codes-config\" option, please see --help.\n");
      return 1;
  }
  if (configuration_load(conf_file_name, &config)) {
      fprintf(stderr, "Error loading config file %s.\n", conf_file_name);
      return 1;
  }

  model_net_register();
  svr_add_lp_type();
  codes_mapping_setup();
  //if (g_st_ev_trace || g_st_model_stats)
  //    svr_register_model_types();

  int num_nets;
  int* net_ids = model_net_configure(&num_nets);
  //assert(num_nets==1);
  net_id = *net_ids;
  free(net_ids);
  assert(net_id == DRAGONFLY);

  /* 5 days of simulation time */
  //g_tw_ts_end = s_to_ns(5 * 24 * 60 * 60);
  //model_net_enable_sampling(sampling_interval, sampling_end_time);

  num_servers_per_rep = codes_mapping_get_lp_count(
      "MODELNET_GRP", 1, "server", NULL, 1);
  configuration_get_value_int(
      &config, "PARAMS", "num_routers", NULL, &num_routers_per_grp);
  num_groups = (num_routers_per_grp * (num_routers_per_grp/2) + 1);
  num_nodes = num_groups * num_routers_per_grp * (num_routers_per_grp / 2);
  num_nodes_per_grp = num_routers_per_grp * (num_routers_per_grp / 2);

  /*if(lp_io_dir[0])
  {
      do_lp_io = 1;
      int flags = lp_io_use_suffix ? LP_IO_UNIQ_SUFFIX : 0;
      int ret = lp_io_prepare(lp_io_dir, flags, &io_handle, MPI_COMM_WORLD);
      assert(ret == 0 || !"lp_io_prepare failure");
  }*/

  tw_run();

  /*if (do_lp_io){
      int ret = lp_io_flush(io_handle, MPI_COMM_WORLD);
      assert(ret == 0 || !"lp_io_flush failure");
  }*/
  //model_net_report_stats(net_id);

  tw_end();

  return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */
