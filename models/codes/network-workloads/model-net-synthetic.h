/* Global configuration variables */
int net_id;

/* Type of events */
enum svr_event {
  KICKOFF,  /* kickoff event */
  REMOTE,   /* remote event */
  LOCAL     /* local event */
};

/* Server state */
struct svr_state {
  int msg_sent_count;     /* requests sent */
  int msg_recvd_count;    /* requests recvd */
  int local_recvd_count;  /* number of local messages received */
  tw_stime start_ts;      /* time that we started sending requests */
  tw_stime end_ts;        /* time that we ended sending requests */
};

/* Server message */
struct svr_msg {
  enum svr_event svr_event_type;
  tw_lpid src;                      /* source of this request or ack */
  int incremented_flag;             /* helper for reverse computation */
  model_net_event_return event_rc;
};

/* Implemented differently for specific topologies and configurations */
tw_stime get_mean_interval();
tw_lpid get_dest(tw_lp* lp);
tw_lpid get_dest_rc(tw_lp* lp);

/* Sends a kickoff event to self based on the model specified mean interval */
void issue_event(svr_state* ns, tw_lp* lp);
void issue_event_rc(svr_state* ns, tw_lp* lp);

/* Base server event handlers */
void svr_init(svr_state* ns, tw_lp* lp);
void svr_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void svr_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void svr_commit_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void svr_finalize(svr_state* ns, tw_lp* lp);

/* Forward and reverse handlers for the different msg types */
void handle_kickoff_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void handle_local_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void handle_remote_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void handle_kickoff_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void handle_local_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);
void handle_remote_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp);

void issue_event(svr_state* ns, tw_lp* lp) {
  (void)ns;
  /* each server sends a dummy event to itself that will kick off the real
   * simulation
   */
  tw_stime kickoff_time = g_tw_lookahead + tw_rand_exponential(lp->rng, get_mean_interval());
  tw_event* e = tw_event_new(lp->gid, kickoff_time, lp);
  svr_msg* m = (svr_msg*)tw_event_data(e);
  m->svr_event_type = KICKOFF;
  tw_event_send(e);
}
void issue_event_rc(svr_state* ns, tw_lp* lp) {
  tw_rand_reverse_unif(lp->rng);
}

tw_lptype svr_lp = {
    (init_f) svr_init,
    (event_f) svr_event,
    (revent_f) svr_rev_event,
    (commit_f) svr_commit_event,
    (final_f)  svr_finalize,
    sizeof(svr_state),
};

void svr_add_lp_type() {
  lp_type_register("server", &svr_lp);
}
tw_stime ns_to_s(tw_stime ns) {
  return ns / (1000.0 * 1000.0 * 1000.0);
}
tw_stime s_to_ns(tw_stime s) {
  return s * (1000.0 * 1000.0 * 1000.0);
}
tw_stime bytes_to_ns(uint64_t bytes, double GB_p_s) {
  /* bytes to GB */
  tw_stime time = ((double)bytes)/(1024.0*1024.0*1024.0);
  /* MB to s */
  time = time / GB_p_s;
  /* s to ns */
  time = time * 1000.0 * 1000.0 * 1000.0;

  return time;
}

void svr_init(svr_state* ns, tw_lp* lp) {
  memset(ns, 0, sizeof(*ns));
  issue_event(ns, lp);
}

void svr_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  switch (m->svr_event_type) {
    case REMOTE:
      handle_remote_event(ns, b, m, lp);
      break;
    case LOCAL:
      handle_local_event(ns, b, m, lp);
      break;
    case KICKOFF:
      handle_kickoff_event(ns, b, m, lp);
      break;
    default:
      CkPrintf("LP %d: Invalid message from src lpID %d of message type %d\n",
          lp->gid, m->src, m->svr_event_type);
      CkAbort("ERROR: Bad server event type in svr_event\n");
      break;
  }
}

void svr_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  switch (m->svr_event_type) {
    case REMOTE:
      handle_remote_rev_event(ns, b, m, lp);
      break;
    case LOCAL:
      handle_local_rev_event(ns, b, m, lp);
      break;
    case KICKOFF:
      handle_kickoff_rev_event(ns, b, m, lp);
      break;
    default:
      CkAbort("ERROR: Bad server event type in svr_rev_event\n");
      break;
  }
}

void svr_commit_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {}

void svr_finalize(svr_state* ns, tw_lp* lp) {
  ns->end_ts = tw_now(lp);
  double duration = ns_to_s(ns->end_ts - ns->start_ts);
  CkPrintf("server %llu recvd %d bytes in %f seconds, \
            %f MiB/s sent_count %d recvd_count %d local_count %d \n",
    lp->gid, PAYLOAD_SZ*ns->msg_recvd_count, duration,
    (double)(PAYLOAD_SZ*ns->msg_sent_count)/(1024.0*1024.0)/duration,
    ns->msg_sent_count, ns->msg_recvd_count, ns->local_recvd_count);
}

void handle_kickoff_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  svr_msg m_local, m_remote;

  m_local.svr_event_type = LOCAL;
  m_local.src = lp->gid;

  memcpy(&m_remote, &m_local, sizeof(svr_msg));
  m_remote.svr_event_type = REMOTE;

  // TODO: I don't think this makes sense with multiple kickoff events
  ns->start_ts = tw_now(lp);

  tw_lpid global_dest = get_dest(lp);
  ns->msg_sent_count++;
  m->event_rc = model_net_event(net_id, "test", global_dest, PAYLOAD_SZ, 0.0,
      sizeof(svr_msg), &m_remote, sizeof(svr_msg), &m_local, lp);
  issue_event(ns, lp);
}
void handle_kickoff_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  get_dest_rc(lp);                        // Reverse get_dest
  ns->msg_sent_count--;                   // Reverse msg count increment
  model_net_event_rc2(lp, &m->event_rc);  // Reverse model_net_event
  issue_event_rc(ns, lp);                 // Reverse issue_event
}

void handle_remote_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  (void)b;
  (void)m;
  (void)lp;
  ns->msg_recvd_count++;
}
void handle_remote_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  (void)b;
  (void)m;
  (void)lp;
  ns->msg_recvd_count--;
}

void handle_local_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  (void)b;
  (void)m;
  (void)lp;
  ns->local_recvd_count++;
}
void handle_local_rev_event(svr_state* ns, tw_bf* b, svr_msg* m, tw_lp* lp) {
  (void)b;
  (void)m;
  (void)lp;
  ns->local_recvd_count--;
}
