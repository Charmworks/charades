#ifndef _BUCKETED_H_
#define _BUCKETED_H_

#include "gvtmanager.h"

#include "event.h"

struct OffsetStats {
  int regular, anti;
  int held, cancelled, released;
  int lag;

  OffsetStats()
      : regular(0), anti(0), held(0), cancelled(0), released(0), lag(0) {}

  double anti_ratio() const {
    return regular ? (double)anti / regular : 0.0;
  }

  double held_ratio() const {
    return regular ? (double)held / regular : 0.0;
  }

  double cancelled_ratio() const {
    return held ? (double)cancelled / held : 0.0;
  }

  double released_ratio() const {
    return held ? (double)released / held : 0.0;
  }

  double average_lag() const {
    return anti ? (double)lag / anti : 0.0;
  }
};

class BucketGVT : public CBase_BucketGVT {
  public:

    BucketGVT();  ///< Allocate arrays and 0 out all state

    void finalize();

    /** Called periodically by scheduler to ensure GVT progress */
    void gvt_begin();

    /** Returns number of buckets the local scheduler instance has passed */
    int buckets_passed() const;

    /**
     * Starts the next GVT computation with a reduction to all_ready if active
     * is false and buckets_passed() is at least 1.
     */
    void attempt_gvt();

    /**
     * Target of reduction signalling all PEs are ready with the minumum number
     * of buckets passed by all schedulers.
     */
    void all_ready(int num_buckets);

    /**
     * Contributes sent and recvd counts for the specified number of buckets, as
     * well as the number of buckets we are still passed, to check_counts().
     */
    void send_counts(int num_buckets);

    /**
     * Target of the tuple reduction from send_counts() which contains the sent
     * and recvd counts for relevant buckets as well as the minimum number of
     * buckets passed by all PEs.
     */
    void check_counts(CkReductionMsg* msg);

    /** Advances the GVT by the specified number of buckets */
    void advance_gvt(int num_buckets);

    /** Attempt to cancel adaptively held back events */
    bool attempt_cancel(RemoteEvent* anti);

    void consume(RemoteEvent* e); ///< Increment recvd and attempt GVT
    bool produce(RemoteEvent* e); ///< Increment sent and attempt GVT

  private:
    Time bucket_size;   ///< Size of each bucket
    Time max_ts;        ///< Max end ts used when bucket size doesn't divide end
    int total_buckets;  ///< Total number of buckets
    int curr_bucket;    ///< Index of current bucket
    int* counts;        ///< Array of sent and received counts

    int start_reductions;
    int count_reductions;

    OffsetStats* stats;
    int reserve_buckets;
    int clear_buckets;
    double reserve_threshold;
    std::vector<RemoteEvent*>* reserves;
};

#endif
