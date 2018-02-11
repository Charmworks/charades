/**
 * Header for declarations for the basic modelnet test harness. Each test sets
 * up a number of servers based on network topology and configuration
 * parameters. Each server is paired with an LP serving as the NIC, and
 * exchanges a sequence of requests and acks with a peer.
 */

// Global variables and defines
#define NUM_REQS 2      /* number of requests sent by each server */
#define PAYLOAD_SZ 4096 /* size of simulated data payload, bytes  */
#define DO_PULL 0       /* whether to pull instead of push */

int net_id = 0;  /* network id (SIMPLENET, SIMPLEP2P, TORUS, etc) */
int num_routers = 0;
int num_servers = 0;
char router_name[MAX_NAME_LENGTH];

// Enums and structs defining a server
enum svr_event {
  KICKOFF,  /* initial event */
  REQ,      /* request event */
  ACK,      /* ack event */
  LOCAL    /* local event */
};

struct svr_state {
    int msg_sent_count;     /* requests sent */
    int msg_recvd_count;    /* requests recvd */
    int local_recvd_count;  /* number of local messages received */
    tw_stime start_ts;      /* time that we started sending requests */
    tw_stime end_ts;        /* time that we ended sending requests */
};

struct svr_msg {
    enum svr_event svr_event_type;  /* type of event */
    tw_lpid src;                    /* src LP of the event */
    model_net_event_return ret;     /* rc for modelnet calls */
    int incremented_flag;           /* helper for reverse computation */
};

// Server LP handlers
void svr_init(svr_state* ns, tw_lp* lp);
void svr_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void svr_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void svr_commit_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void svr_finalize(svr_state* ns, tw_lp* lp);

// Server LP type
tw_lptype svr_lp = {
    (init_f) svr_init,
    (event_f) svr_event,
    (revent_f) svr_rev_event,
    (commit_f) svr_commit_event,
    (final_f)  svr_finalize,
    sizeof(svr_state),
};

// Helper functions for the LP handlers
void handle_kickoff_event(svr_state* ns, svr_msg* m, tw_lp* lp);
void handle_req_event(svr_state* ns, svr_msg* m, tw_lp* lp);
void handle_ack_event(svr_state* ns, svr_msg* m, tw_lp* lp);
void handle_local_event(svr_state* ns);

void handle_kickoff_rev_event(svr_state* ns, svr_msg* m, tw_lp* lp);
void handle_req_rev_event(svr_state* ns, svr_msg* m, tw_lp* lp);
void handle_ack_rev_event(svr_state* ns, svr_msg* m, tw_lp* lp);
void handle_local_rev_event(svr_state* ns);

// Utility functions
void svr_add_lp_type() {
  lp_type_register("server", &svr_lp);
}
tw_stime ns_to_s(tw_stime ns) {
  return ns / (1000.0 * 1000.0 * 1000.0);
}
tw_stime s_to_ns(tw_stime s) {
  return s * (1000.0 * 1000.0 * 1000.0);
}
// Returns the global ID of the next server
int get_next_server(int from_gid) {
  static int num_servers_per_rep = codes_mapping_get_lp_count(
      "MODELNET_GRP", 1, "server", NULL, 1);
  static int num_routers_per_rep = codes_mapping_get_lp_count(
      "MODELNET_GRP", 1, router_name, NULL, 1);
  static int lps_per_rep = num_servers_per_rep * 2 + num_routers_per_rep;
  static int total_lps = num_servers * 2 + num_routers;

  if (net_id == SIMPLEP2P) {
    return 4; // The SIMPLEP2P configuration is only set up to recv on LP 4
  } else {
    int offset;
    // DRAGONFLY and SLIMFLY require a larger offset between some LPs
    if (net_id == DRAGONFLY || net_id == SLIMFLY) {
      if (from_gid % lps_per_rep == num_servers_per_rep - 1) {
        offset = 1 + num_servers_per_rep + num_routers_per_rep;
      } else {
        offset = 1;
      }
    } else {
      offset = 2;
    }
    return (from_gid + offset) % total_lps;
  }
}
