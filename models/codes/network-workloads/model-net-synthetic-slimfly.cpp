/**
 * Copyright (C) 2015 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

/**
 * The test program generates some synthetic traffic patterns for the model-net
 * network models. Currently it only support the slimfly network model uniform
 * random and nearest neighbor traffic patterns.
*/

#include "codes/model-net.h"
#include "codes/lp-io.h"
#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"

#define PAYLOAD_SZ 256
#include "model-net-synthetic.h"

#define PARAMS_LOG 0
#define LP_CONFIG_NM (model_net_lp_config_names[SLIMFLY])
#define PRINT_WORST_CASE_MATCH 0

static int num_servers_per_rep = 0;
static int num_routers_per_grp = 0;
static int num_nodes_per_grp = 0;
static int num_groups = 0;
static int num_nodes = 0;
static int num_terminals = 0;
static int num_routers = 0;

/*FILE* slimfly_results_log_2=NULL;
FILE* slimfly_ross_csv_log=NULL;
static char lp_io_dir[356] = {'\0'};
static lp_io_handle io_handle;
static unsigned int lp_io_use_suffix = 0;
static int do_lp_io = 0;*/

/* type of synthetic traffic */
enum TRAFFIC {
  UNIFORM = 1,          /* sends message to a randomly selected node */
  WORST_CASE = 2,
  NEAREST_GROUP = 3,    /* sends message to the node connected to the neighboring router */
  NEAREST_NEIGHBOR = 4  /* sends message to the next node (potentially connected to the same router) */
};

/* Command line configuration options */
static int traffic = 1;
static double arrival_time = 1000.0;
static double load = 0.0;
static char conf_file_name[128] = {'\0'};
const tw_optdef app_opt [] = {
    TWOPT_GROUP("Model net synthetic traffic " ),
    TWOPT_UINT("traffic", traffic, "UNIFORM RANDOM=1, NEAREST NEIGHBOR=2 "),
    TWOPT_STIME("arrival_time", arrival_time, "INTER-ARRIVAL TIME"),
    TWOPT_STIME("load", load, "percentage of packet inter-arrival rate to simulate"),
    //TWOPT_CHAR("lp-io-dir", lp_io_dir, "Where to place io output (unspecified -> no output"),
    //TWOPT_UINT("lp-io-use-suffix", lp_io_use_suffix, "Whether to append uniq suffix to lp-io directory (default 0)"),
    TWOPT_CHAR("codes-config", conf_file_name, "name of codes configuration file"),
    TWOPT_END(),
};

/**
 * Latest implementation of function to return an array mapping each router with
 * it's corresponding worst-case router pair.
 */
int* worst_dest; //Array mapping worst case destination for each router
void init_worst_case_mapping() {
  int i,j,k;
  int r1,r2;    //Routers to be paired
  for(k = 0; k < 2; k++) {
    for(j = 0; j < num_routers_per_grp - 1; j += 2) {
      for(i = 0; i < num_routers_per_grp; i++) {
        r1 = i + j*num_routers_per_grp + k*num_routers_per_grp*num_routers_per_grp;
        r2 = i + (j+1)*num_routers_per_grp + k*num_routers_per_grp*num_routers_per_grp;
        worst_dest[r1] = r2;
        worst_dest[r2] = r1;
      }
    }
  }
  j = num_routers_per_grp-1;
  for(i = 0; i < num_routers_per_grp; i++) {
    r1 = i + j*num_routers_per_grp + 0*num_routers_per_grp*num_routers_per_grp;
    r2 = i + j*num_routers_per_grp + 1*num_routers_per_grp*num_routers_per_grp;
    worst_dest[r1] = r2;
    worst_dest[r2] = r1;
  }
}

double MEAN_INTERVAL = 0.0;
tw_stime get_mean_interval() {
  return MEAN_INTERVAL;
}

tw_lpid get_dest(tw_lp* lp) {
  char group_name[MAX_NAME_LENGTH];
  char lp_type_name[MAX_NAME_LENGTH];
  int group_index, lp_type_index, rep_id, offset;
  char anno[MAX_NAME_LENGTH];
  tw_lpid local_dest, global_dest;

  codes_mapping_get_lp_info(lp->gid, group_name, &group_index, lp_type_name, &lp_type_index, anno, &rep_id, &offset);

  if (traffic == UNIFORM) {
    local_dest = tw_rand_integer(lp->rng, 0, num_nodes - 1);
  } else if (traffic == WORST_CASE) {
    int num_lps = codes_mapping_get_lp_count(group_name, 1, LP_CONFIG_NM, anno, 0);
    int src_terminal_id = (rep_id * num_lps) + offset;
    int src_router_id = src_terminal_id / num_lps;
    int dst_router_id = worst_dest[src_router_id];
    local_dest = num_lps * dst_router_id + (src_terminal_id % num_terminals);
  } else if (traffic == NEAREST_GROUP) {
    local_dest = (rep_id * 2 + offset + num_nodes_per_grp) % num_nodes;
  } else if (traffic == NEAREST_NEIGHBOR) {
    local_dest =  (rep_id * 2 + offset + 2) % num_nodes;
  } else {
    CkAbort("ERROR: Invalid value for traffic\n");
  }
  assert(local_dest < num_nodes);

  codes_mapping_get_lp_id(group_name, lp_type_name, anno, 1, local_dest / num_servers_per_rep, local_dest % num_servers_per_rep, &global_dest);
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

  if (!conf_file_name[0]){
      fprintf(stderr, "Expected \"codes-config\" option, please see --help.\n");
      return 1;
  }
  if (configuration_load(conf_file_name, &config)){
      fprintf(stderr, "Error loading config file %s.\n", conf_file_name);
      return 1;
  }

  model_net_register();
  svr_add_lp_type();
  codes_mapping_setup();

  int num_nets;
  int* net_ids = model_net_configure(&num_nets);
  assert(num_nets==1);
  net_id = *net_ids;
  free(net_ids);
  assert(net_id == SLIMFLY);

  num_servers_per_rep = codes_mapping_get_lp_count(
      "MODELNET_GRP", 1, "server", NULL, 1);
  configuration_get_value_int(
      &config, "PARAMS", "num_terminals", NULL, &num_terminals);
  configuration_get_value_int(
      &config, "PARAMS", "num_routers", NULL, &num_routers_per_grp);
  num_groups = num_routers_per_grp * 2;
  num_nodes = num_groups * num_routers_per_grp * num_servers_per_rep;
  num_nodes_per_grp = num_routers_per_grp * num_servers_per_rep;
  num_routers = num_routers_per_grp * num_routers_per_grp * 2;

  int this_packet_size = 0;
  double this_global_bandwidth = 0.0;
  configuration_get_value_int(
      &config, "PARAMS", "packet_size", NULL, &this_packet_size);
  configuration_get_value_double(
      &config, "PARAMS", "global_bandwidth", NULL, &this_global_bandwidth);

  if (!this_packet_size) {
    CkAbort("Configuration error: packet size not specified\n");
  }
  if (!this_global_bandwidth) {
    this_global_bandwidth = 4.7;
    CkPrintf("Bandwidth of global channels not specified, setting to %lf\n",
        this_global_bandwidth);
  }

  if (load != 0) {
    MEAN_INTERVAL = bytes_to_ns(this_packet_size, load * this_global_bandwidth);
  } else if (arrival_time != 0) {
    MEAN_INTERVAL = arrival_time;
  }

  /*if (lp_io_prepare("modelnet-test", LP_IO_UNIQ_SUFFIX, &handle, MPI_COMM_WORLD) < 0) {
    return(-1);
  }

  if (lp_io_dir[0]) {
    do_lp_io = 1;
    int flags = lp_io_use_suffix ? LP_IO_UNIQ_SUFFIX : 0;
    int ret = lp_io_prepare(lp_io_dir, flags, &io_handle, MPI_COMM_WORLD);
    assert(ret == 0 || !"lp_io_prepare failure");
  }*/

  //WORST_CASE Initialization array
  if (traffic == WORST_CASE) {
    worst_dest = (int*)calloc(num_routers,sizeof(int));
    init_worst_case_mapping();
#if PRINT_WORST_CASE_MATCH
    for(int l = 0; l < num_routers; l++) {
      CkPrintf("match %d->%d\n",l,worst_dest[l]);
    }
#endif
  }

  tw_run();

  /*if (do_lp_io) {
    int ret = lp_io_flush(io_handle, MPI_COMM_WORLD);
    assert(ret == 0 || !"lp_io_flush failure");
  }
  model_net_report_stats(net_id);*/

  if (CkMyPe() == 0) {
#if PARAMS_LOG
    //Open file to append simulation results
    char log[200];
    sprintf( log, "slimfly-results-log.txt");
    slimfly_results_log_2=fopen(log, "a");
    if(slimfly_results_log_2 == NULL) {
      tw_error(TW_LOC, "\n Failed to open slimfly results log file \n");
    }
    printf("Printing Simulation Parameters/Results Log File\n");
    fprintf(slimfly_results_log_2,"%16.3d, %6.2f, %13.2f, ",traffic, load, MEAN_INTERVAL);
#endif
  }

  /*if (lp_io_flush(handle, MPI_COMM_WORLD) < 0) {
    assert(ret == 0 || !"lp_io_flush failure");
    return -1;
  }*/

  tw_end();

  if (CkMyPe() == 0) {
#if PARAMS_LOG
    slimfly_ross_csv_log=fopen("ross.csv", "r");
    if(slimfly_ross_csv_log == NULL) {
      tw_error(TW_LOC, "\n Failed to open ross.csv log file \n");
    }
    printf("Reading ROSS specific data from ross.csv and Printing to Slim Fly Log File\n");

    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, slimfly_ross_csv_log);
    while (read != -1) {
      read = getline(&line, &len, slimfly_ross_csv_log);
    }
    //read = getline(&line, &len, slimfly_ross_csv_log);

    char* pch;
    pch = strtok (line,",");
    int idx = 0;
    int gvt_computations;
    long long total_events, rollbacks, net_events;
    float running_time, efficiency, event_rate;
    while (pch != NULL) {
      pch = strtok (NULL, ",");
      //printf("%d: %s\n",idx,pch);
      switch(idx) {
        /*case 0:
          printf("%s\n",pch);
          break;
        case 1:
          printf("%s\n",pch);
          break;
        case 2:
          printf("%s\n",pch);
          break;
        case 3:
          printf("%s\n",pch);
          break;
        case 4:
          printf("%s\n",pch);
          break;*/
        case 4:
          total_events = atoll(pch);
          break;
        case 13:
          rollbacks = atoll(pch);
          break;
        case 17:
          gvt_computations = atoi(pch);
          break;
        case 18:
          net_events = atoll(pch);
          break;
        case 3:
          running_time = atof(pch);
          break;
        case 8:
          efficiency = atof(pch);
          break;
        case 19:
          event_rate = atof(pch);
          break;
      }
      idx++;
    }
    fprintf(slimfly_results_log_2,"%12llu, %10llu, %16d, %10llu, %17.4f, %10.2f, %22.2f\n",total_events,rollbacks,gvt_computations,net_events,running_time,efficiency,event_rate);
    fclose(slimfly_results_log_2);
    fclose(slimfly_ross_csv_log);
#endif
  }
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
