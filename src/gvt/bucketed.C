#include "bucketed.h"

#include "event.h"
#include "globals.h"
#include "scheduler.h"

#include <float.h>

BucketGVT::BucketGVT() {
  gvt_name = "Bucket GVT";
  active = false;
  continuous = true;

  bucket_size = g_tw_gvt_bucket_size;

  int num_buckets = ceil(g_tw_ts_end / (double) bucket_size);
  cur_bucket = 0;
  rollback_flags = new int[num_buckets];
  sent = new int[num_buckets];
  received = new int[num_buckets];
  min_sent = new Time[num_buckets];
  doing_reduction = false;
  for (int i = 0; i < num_buckets; i++) {
    rollback_flags[i] = 0;
    sent[i] = 0;
    received[i] = 0;
    min_sent[i] = DBL_MAX;
  }
}

// TODO: POSSIBLY STILL NEEDED AS A CHECK FOR CASES WHERE NO PROD/CONS HAPPENS
void BucketGVT::gvt_begin() {
  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
    active = true;
  }
  scheduler->gvt_resume();
}

// TODO: Can we just check min time here? We may not need rollback_flags
// Unless its the case that rolling back, then advance past bucket time is important to track
void BucketGVT::bucket_ready() {
  int data[3] = { sent[cur_bucket], received[cur_bucket], rollback_flags[cur_bucket] };
  contribute(3 * sizeof(int), data, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT,check_counts),thisProxy));
}

void BucketGVT::check_counts(int sent, int recvd, int flag) {
  if (flag != 0) {
    active = false;
    doing_reduction = false;
    if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size) {
      rollback_flags[cur_bucket] = 0;
      active = true;
      doing_reduction = true;
      contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
    }
  } else if (sent != recvd) {
    bucket_ready();
  } else {
    contribute(sizeof(int), &rollback_flags[cur_bucket], CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT,gvt_end),thisProxy));
  }
}

void BucketGVT::gvt_end(int flag) {
  if (flag == 0) {
    prev_gvt = curr_gvt;
    curr_gvt = ++cur_bucket * bucket_size;
    active = false;
    doing_reduction = false;
    scheduler->gvt_done(curr_gvt);
  } else if (scheduler->get_min_time() > (cur_bucket + 1)  * bucket_size) {
    rollback_flags[cur_bucket] = 0;
    active = true;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  }
}

void BucketGVT::consume(RemoteEvent* e) {
  received[e->phase]++;

  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    active = true;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  } else if(scheduler->get_min_time() < (1 + cur_bucket) * bucket_size) {
    rollback_flags[cur_bucket] = 1;
  }
}

void BucketGVT::produce(RemoteEvent* e) {
  e->phase = e->ts / bucket_size;
  sent[e->phase]++;

  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    active = true;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  }
}
