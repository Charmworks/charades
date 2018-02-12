/**
 * Copyright (C) 2015 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

/**
 * The test program generates some synthetic traffic patterns for the model-net
 * network models. Currently it only support the fat tree network model uniform
 * random traffic pattern.
 */

#include "codes/model-net.h"
#include "codes/lp-io.h"
#include "codes/net/fattree.h"
#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"

#define PAYLOAD_SZ 512
#include "model-net-synthetic.h"

#define PARAMS_LOG 0

static int num_servers_per_rep = 0;
static int num_routers_per_grp = 0;
static int num_nodes_per_grp = 0;
static int num_groups = 0;
static int num_nodes = 0;

/* type of synthetic traffic */
enum TRAFFIC {
  UNIFORM = 1,          /* sends message to a randomly selected node */
  NEAREST_GROUP = 2,    /* sends message to the node connected to the neighboring router */
  NEAREST_NEIGHBOR = 3  /* sends message to the next node (potentially connected to the same router) */
};

#if 0 // TRACING CODE NOT ENABLED
char* modelnet_stats_dir;
/* setup for the ROSS event tracing
 * can have a different function for  rbev_trace_f and ev_trace_f
 * but right now it is set to the same function for both
 */
void ft_svr_event_collect(
    svr_msg *m, tw_lp *lp, char *buffer, int *collect_flag) {
  (void)lp;
  (void)collect_flag;

  int type = (int) m->svr_event_type;
  memcpy(buffer, &type, sizeof(type));
}

/* can add in any model level data to be collected along with simulation engine data
 * in the ROSS instrumentation.  Will need to update the last field in
 * ft_svr_model_types[0] for the size of the data to save in each function call
 */
void ft_svr_model_stat_collect(svr_state *s, tw_lp *lp, char *buffer) {
  (void)s;
  (void)lp;
  (void)buffer;
}

st_model_types ft_svr_model_types[] = {
  {
    (rbev_trace_f) ft_svr_event_collect,
    sizeof(int),
    (ev_trace_f) ft_svr_event_collect,
    sizeof(int),
    (model_stat_f) ft_svr_model_stat_collect,
    0
  },
  {NULL, 0, NULL, 0, NULL, 0}
};

static const st_model_types  *ft_svr_get_model_stat_types(void) {
  return(&ft_svr_model_types[0]);
}

void ft_svr_register_model_stats() {
  st_model_type_register("server", ft_svr_get_model_stat_types());
}
#endif

/* Command line configuration options */
static int traffic = 1;
static double arrival_time = 1000.0;
static double load = 0.0;
static char conf_file_name[128] = {'\0'};
const tw_optdef app_opt [] = {
  TWOPT_GROUP("Model net synthetic traffic " ),
  TWOPT_UINT("traffic", traffic, "UNIFORM RANDOM=1, NEAREST NEIGHBOR=2 "),
  TWOPT_STIME("arrival_time", arrival_time, "INTER-ARRIVAL TIME"),
  TWOPT_STIME("load", load, "percentage of terminal link bandiwdth to inject packets"),
  TWOPT_CHAR("codes-config", conf_file_name, "name of codes configuration file"),
  TWOPT_END()
};

static double MEAN_INTERVAL = 0.0;
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
  // In the case of uniform random traffic, send to a random destination
  if (traffic == UNIFORM) {
    local_dest = tw_rand_integer(lp->rng, 0, num_nodes - 1);
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
  //if (g_st_ev_trace)
  //    ft_svr_register_model_stats();

  int num_nets;
  int* net_ids = model_net_configure(&num_nets);
  assert(num_nets==1);
  net_id = *net_ids;
  free(net_ids);
  assert(net_id == FATTREE);

  num_servers_per_rep = codes_mapping_get_lp_count(
      "MODELNET_GRP", 1, "server", NULL, 1);
  num_nodes = codes_mapping_get_lp_count(
      "MODELNET_GRP", 0, "server", NULL, 1);
  configuration_get_value_int(
      &config, "PARAMS", "num_routers", NULL, &num_routers_per_grp);
  num_groups = (num_routers_per_grp * (num_routers_per_grp/2) + 1);
  num_nodes_per_grp = num_routers_per_grp * (num_routers_per_grp / 2);

  int this_packet_size = 0;
  double this_link_bandwidth = 0.0;
  configuration_get_value_int(
      &config, "PARAMS", "packet_size", NULL, &this_packet_size);
  configuration_get_value_double(
      &config, "PARAMS", "link_bandwidth", NULL, &this_link_bandwidth);

  if (!this_packet_size) {
    CkAbort("Configuration error: packet size not specified\n");
  }
  if (!this_link_bandwidth) {
    this_link_bandwidth = 4.7;
    CkPrintf("Bandwidth of channels not specified, setting to %lf\n",
        this_link_bandwidth);
  }

  if(load != 0) {
    MEAN_INTERVAL = bytes_to_ns(this_packet_size, load*this_link_bandwidth);
  } else if(arrival_time != 0) {
    MEAN_INTERVAL = arrival_time;
  }

  /*lp_io_handle handle;
  if (lp_io_prepare("modelnet-test", LP_IO_UNIQ_SUFFIX, &handle, MPI_COMM_WORLD) < 0) {
      return(-1);
  }
  modelnet_stats_dir = lp_io_handle_to_dir(handle);*/

  tw_run();

  //model_net_report_stats(net_id);

#if PARAMS_LOG
  if (!g_tw_mynode) {
    char temp_filename[1024];
    char temp_filename_header[1024];
    sprintf(temp_filename,"%s/sim_log.txt",modelnet_stats_dir);
    sprintf(temp_filename_header,"%s/sim_log_header.txt",modelnet_stats_dir);
    FILE *fattree_results_log=fopen(temp_filename, "a");
    FILE *fattree_results_log_header=fopen(temp_filename_header, "a");
    if(fattree_results_log == NULL) {
      printf("\n Failed to open results log file %s in synthetic-fattree\n",temp_filename);
    }
    if(fattree_results_log_header == NULL) {
      printf("\n Failed to open results log header file %s in synthetic-fattree\n",temp_filename_header);
    }
    printf("Printing Simulation Parameters/Results Log File\n");
    fprintf(fattree_results_log_header,", <Workload>, <Load>, <Mean Interval>, ");
    fprintf(fattree_results_log,"%11.3d, %5.2f, %15.2f, ",traffic, load, MEAN_INTERVAL);
    fclose(fattree_results_log_header);
    fclose(fattree_results_log);
  }
#endif

  /*if(lp_io_flush(handle, MPI_COMM_WORLD) < 0) {
      return(-1);
  }*/

  tw_end();

#if PARAMS_LOG
  if(!g_tw_mynode) {
    char temp_filename[1024];
    char temp_filename_header[1024];
    sprintf(temp_filename,"%s/sim_log.txt",modelnet_stats_dir);
    sprintf(temp_filename_header,"%s/sim_log_header.txt",modelnet_stats_dir);
    FILE *fattree_results_log=fopen(temp_filename, "a");
    FILE *fattree_results_log_header=fopen(temp_filename_header, "a");
    FILE *fattree_ross_csv_log=fopen("ross.csv", "r");
    if(fattree_results_log == NULL) {
      printf("\n Failed to open results log file %s in synthetic-fattree\n",temp_filename);
    }
    if(fattree_results_log_header == NULL) {
      printf("\n Failed to open results log header file %s in synthetic-fattree\n",temp_filename_header);
    }
    if(fattree_ross_csv_log == NULL) {
      tw_error(TW_LOC, "\n Failed to open ross.csv log file \n");
    }
    printf("Reading ROSS specific data from ross.csv and Printing to Fat Tree Log File\n");

    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, fattree_ross_csv_log);
    while (read != -1)  {
      read = getline(&line, &len, fattree_ross_csv_log);
    }

    char* pch;
    pch = strtok (line,",");
    int idx = 0;
    int gvt_computations;
    long long total_events, rollbacks, net_events;
    float running_time, efficiency, event_rate;
    while (pch != NULL) {
      pch = strtok (NULL, ",");
      switch(idx) {
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
    fprintf(fattree_results_log_header,"<Total Events>, <Rollbacks>, <GVT Computations>, <Net Events>, <Running Time>, <Efficiency>, <Event Rate>");
    fprintf(fattree_results_log,"%14llu, %11llu, %18d, %12llu, %14.4f, %12.2f, %12.2f\n",total_events,rollbacks,gvt_computations,net_events,running_time,efficiency,event_rate);
    fclose(fattree_results_log);
    fclose(fattree_results_log_header);
    fclose(fattree_ross_csv_log);
  }
#endif
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
