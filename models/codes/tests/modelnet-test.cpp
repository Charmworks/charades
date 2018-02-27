/**
 * Copyright (C) 2013 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 */

/**
 * Header for declarations for the basic modelnet test harness. Each test sets
 * up a number of servers based on network topology and configuration
 * parameters. Each server is paired with an LP serving as the NIC, and
 * exchanges a sequence of requests and acks with a peer.
 */

#include <string.h>
#include <assert.h>
#include <ross.h>

#include "codes/model-net.h"
#include "codes/lp-io.h"
#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"

#include "modelnet-test.h"

static char conf_file_name[128] = {'\0'};
const tw_optdef app_opt [] = {
    TWOPT_GROUP("Model net test case" ),
    TWOPT_CHAR("codes-config", conf_file_name, "name of codes configuration file"),
    TWOPT_END()
};

int main(int argc, char** argv) {
  tw_opt_add(app_opt);
  tw_init(argc, argv);

  if (!conf_file_name[0]){
    CkAbort("ERROR: Need a \"codes-config\" option, please see --help.\n");
  }
  if (configuration_load(conf_file_name, &config)){
    CkPrintf("Error loading config file %s.\n", conf_file_name);
    CkAbort("ERROR: Cannot read configuration file.\n");
  }

  int rank = CkMyPe();
  int nprocs = CkNumPes();
  g_tw_ts_end = s_to_ns(60*60*24*365); /* one year, in nsecs */

  model_net_register();
  svr_add_lp_type();
  codes_mapping_setup();

  int num_nets;
  int* net_ids = model_net_configure(&num_nets);
  assert(num_nets >= 1);
  net_id = *net_ids;
  free(net_ids);

  num_servers = codes_mapping_get_lp_count(
      "MODELNET_GRP", 0, "server", NULL, 1);

  if (net_id == DRAGONFLY) {
    strcpy(router_name, "modelnet_dragonfly_router");
    num_routers = codes_mapping_get_lp_count(
        "MODELNET_GRP", 0, router_name, NULL, 1);
  } else if (net_id == SLIMFLY) {
    strcpy(router_name, "slimfly_router");
    num_routers = codes_mapping_get_lp_count(
        "MODELNET_GRP", 0, router_name, NULL, 1);
  } else if (net_id == SIMPLEP2P) {
    assert(num_servers == 3);
  }

  /*lp_io_handle handle;
  if(lp_io_prepare("modelnet-test", LP_IO_UNIQ_SUFFIX, &handle, MPI_COMM_WORLD) < 0)
  {
      return(-1);
  }*/

  tw_run();
  model_net_report_stats(net_id);

  /*if(lp_io_flush(handle, MPI_COMM_WORLD) < 0)
  {
      return(-1);
  }*/

  tw_end();
  return 0;
}

/**
 * Each server sends a dummy event to itself that will kick off the real work
 */
void svr_init(svr_state* ns, tw_lp* lp) {
  memset(ns, 0, sizeof(*ns));

  tw_stime kickoff_time = g_tw_lookahead + tw_rand_unif(lp->rng);
  tw_event* e = tw_event_new(lp->gid, kickoff_time, lp);
  svr_msg* m = (svr_msg*)tw_event_data(e);
  m->svr_event_type = KICKOFF;
  tw_event_send(e);
}

void svr_pup(svr_state* ns, tw_lp* lp, PUP::er& p) {
  p | ns->msg_sent_count;
  p | ns->msg_recvd_count;
  p | ns->local_recvd_count;
  p | ns->start_ts;
  p | ns->end_ts;
}

void svr_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  (void)b;
  switch (m->svr_event_type) {
    case REQ:
      handle_req_event(ns, m, lp);
      break;
    case ACK:
      handle_ack_event(ns, m, lp);
      break;
    case KICKOFF:
      handle_kickoff_event(ns, m, lp);
      break;
    case LOCAL:
      handle_local_event(ns);
      break;
    default:
      CkPrintf("\n Invalid message type %d ", m->svr_event_type);
      CkAbort("ERROR in svr_event\n");
      break;
  }
}

void svr_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  (void)b;
  switch (m->svr_event_type) {
    case REQ:
      handle_req_rev_event(ns, m, lp);
      break;
    case ACK:
      handle_ack_rev_event(ns, m, lp);
      break;
    case KICKOFF:
      handle_kickoff_rev_event(ns, m, lp);
      break;
    case LOCAL:
      handle_local_rev_event(ns);
      break;
    default:
      CkPrintf("\n Invalid message type %d ", m->svr_event_type);
      CkAbort("ERROR in svr_rev_event\n");
      break;
  }
}

void svr_commit_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {}

void svr_finalize(svr_state* ns, tw_lp* lp) {
  double duration = ns_to_s(ns->end_ts - ns->start_ts);
  CkPrintf("server %llu recvd %d bytes in %f seconds, \
            %f MiB/s sent_count %d recvd_count %d local_count %d\n",
      lp->gid, PAYLOAD_SZ*ns->msg_recvd_count, duration,
      (double)(PAYLOAD_SZ*NUM_REQS)/(1024.0*1024.0)/duration,
      ns->msg_sent_count, ns->msg_recvd_count, ns->local_recvd_count);
}

/* handle initial event */
void handle_kickoff_event(svr_state* ns, svr_msg* m, tw_lp* lp) {
  svr_msg m_local, m_remote;

  m_local.svr_event_type = LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(svr_msg));
  m_remote.svr_event_type = REQ;

  /* record when transfers started on this server */
  ns->start_ts = tw_now(lp);

  /* each server sends a request to the next highest server */
  int dest_id = get_next_server(lp->gid);
  if (DO_PULL) {
    m->ret = model_net_pull_event(net_id, "test", dest_id, PAYLOAD_SZ, 0.0,
        sizeof(svr_msg), &m_remote, lp);
  } else {
    m->ret = model_net_event(net_id, "test", dest_id, PAYLOAD_SZ, 0.0,
        sizeof(svr_msg), &m_remote, sizeof(svr_msg), &m_local, lp);
  }
  ns->msg_sent_count++;
}
/* reverse handler for kickoff */
void handle_kickoff_rev_event(svr_state* ns, svr_msg* m, tw_lp* lp) {
  model_net_event_rc2(lp, &m->ret);
  ns->msg_sent_count--;
}

/* handle receiving request
 * (note: this should never be called when doing the "pulling" version of
 * the program) */
void handle_req_event(svr_state* ns, svr_msg* m, tw_lp* lp) {
  assert(!DO_PULL);
  assert(lp->gid == get_next_server(m->src));
  svr_msg m_local, m_remote;

  m_local.svr_event_type = LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(svr_msg));
  m_remote.svr_event_type = ACK;

  /* send ack back */
  /* simulated payload of 1 MiB */
  /* also trigger a local event for completion of payload msg */
  /* remote host will get an ack event */
  m->ret = model_net_event(net_id, "test", m->src, PAYLOAD_SZ, 0.0,
      sizeof(svr_msg), &m_remote, sizeof(svr_msg), &m_local, lp);
  ns->msg_recvd_count++;
}
void handle_req_rev_event(svr_state* ns, svr_msg* m, tw_lp* lp) {
  model_net_event_rc2(lp, &m->ret);
  ns->msg_recvd_count--;
}

/* handle recving ack */
void handle_ack_event(svr_state* ns, svr_msg* m, tw_lp* lp) {
  svr_msg m_local, m_remote;

  m_local.svr_event_type = LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(svr_msg));
  m_remote.svr_event_type = REQ;

  int dest_id = get_next_server(lp->gid);
  /* in the "pull" case, src should actually be self */
  if (DO_PULL) {
    assert(m->src == lp->gid);
  } else {
    assert(m->src == dest_id);
  }

  if(ns->msg_sent_count < NUM_REQS) {
    /* send another request */
    if (DO_PULL) {
      m->ret = model_net_pull_event(net_id, "test", dest_id, PAYLOAD_SZ, 0.0,
          sizeof(svr_msg), &m_remote, lp);
    } else {
      m->ret = model_net_event(net_id, "test", dest_id, PAYLOAD_SZ, 0.0,
      sizeof(svr_msg), &m_remote, sizeof(svr_msg), &m_local, lp);
    }
    ns->msg_sent_count++;
    m->incremented_flag = 1;
  } else {
    ns->end_ts = tw_now(lp);
    m->incremented_flag = 0;
  }
}
/* reverse handler for ack*/
void handle_ack_rev_event(svr_state* ns, svr_msg* m, tw_lp* lp) {
  if(m->incremented_flag) {
      model_net_event_rc2(lp, &m->ret);
      ns->msg_sent_count--;
  }
  // don't worry about resetting end_ts - just let the ack
  // event bulldoze it
}

void handle_local_event(svr_state* ns) {
  ns->local_recvd_count++;
}
void handle_local_rev_event(svr_state* ns) {
  ns->local_recvd_count--;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ft=c ts=8 sts=4 sw=4 expandtab
 */
