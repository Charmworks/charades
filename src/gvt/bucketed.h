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

    /** Calls attempt_gvt and Scheduler::gvt_resume */
    void gvt_begin();
    /**
     * If invalid, then even though sent == received some PE has rolled back to
     * within the current bucket, so reattempt the GVT. Otherwise, GVT is the
     * end of the bucket, so advance bucket and call Scheduler::gvt_done.
     */
    void gvt_end(int count, int* invalid);  ///< If !invalid set new GVT and advance bucket

    /** Returns 1 if the scheduler has passed the end of the current bucket */
    int buckets_passed() const;

    /**
     * Starts the GVT with a reduction to all_ready if passed_bucket() returns
     * 1 and active is false. Called whenever a state update might mean it's
     * time for a GVT computation, not just from gvt_begin.
     */
    void attempt_gvt();

    bool attempt_cancel(RemoteEvent* anti);

    /** Target of reduction signalling all PEs are ready, so call send_counts */
    void all_ready(int min);
    /** Contributes sent/recvd counts to sum redn along with validity bit */
    void send_counts(int buckets);

    /**
     * Reduction target that receives counts of all sent and received messages.
     * If invalid is non-zero it means at least one PE has rolled back to within
     * the bucket, so we can't continue yet and instead reattempt the GVT.
     * As long as invalid remains 0, call send_counts until sent == recvd. Then
     * trigger one last reduction to gvt_end to catch any last rollbacks.
     */
    void check_counts(int count, int* data);

    void consume(RemoteEvent* e); ///< Increment recvd and attempt GVT
    bool produce(RemoteEvent* e); ///< Increment sent and attempt GVT

  private:
    Time bucket_size;   ///< Size of each bucket
    int total_buckets;  ///< Total number of buckets
    int curr_bucket;    ///< Index of current bucket
    // TODO: We can make this a single array to avoid exra allocation for redns
    int* sent;          ///< Array of sent counts
    int* received;      ///< Array of received counts

    OffsetStats* stats;
    int reserve_buckets;
    double reserve_threshold;
    std::vector<RemoteEvent*>* reserves;
};

#endif
