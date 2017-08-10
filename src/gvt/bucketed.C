#include "bucketed.h"

#include "event.h"
#include "globals.h"
#include "scheduler.h"

#include <float.h>

BucketGVT::BucketGVT() {
  gvt_name = "Bucket GVT";

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
    min_sent[i] = TIME_MAX;
  }
}

void BucketGVT::gvt_begin() {

  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  }
  scheduler->gvt_resume();

}

void BucketGVT::bucket_ready() {

  int data[3] = {sent[cur_bucket], received[cur_bucket], rollback_flags[cur_bucket]};

  contribute(3 * sizeof(int), data, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT,check_counts),thisProxy));

}

void BucketGVT::check_counts(int sent, int recvd, int flag ) {

  if(flag != 0) {

    doing_reduction = false;
    if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size) {
      rollback_flags[cur_bucket] = 0;
      doing_reduction = true;
      contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
    }
  }
  else if (sent != recvd) {

    bucket_ready();
  }
  else {

    Time min_time = scheduler->get_min_time();
    min_time = std::min(min_time, min_sent[cur_bucket]);
    CkAssert(min_time >= curr_gvt);

    // TODO: This is weird, should use Time typedef
    uint64_t flag = ((rollback_flags[cur_bucket]) ? 0 : 1);
    uint64_t data[2] = {min_time, flag};

    contribute(2 * sizeof(double), data, CkReduction::min_ulong_long,
      CkCallback(CkReductionTarget(BucketGVT,gvt_end),thisProxy));
  }
}

void BucketGVT::gvt_end(Time new_gvt, double flag) {
  if (flag == 0) {
    doing_reduction = false;
    if(scheduler->get_min_time() > (cur_bucket + 1)  * bucket_size) {
      rollback_flags[cur_bucket] = 0;
      doing_reduction = true;
      contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
    }
  }
  else {
    prev_gvt = curr_gvt;
    curr_gvt = new_gvt;
    cur_bucket++;
    doing_reduction = false;
    scheduler->gvt_done(curr_gvt);
  }
}

void BucketGVT::consume(RemoteEvent* e) {
  received[e->phase]++;

  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  }
  else if(scheduler->get_min_time() < (1 + cur_bucket) * bucket_size && doing_reduction) {
    rollback_flags[cur_bucket] = 1;
  }
}

void BucketGVT::produce(RemoteEvent* e) {

  e->phase = e->ts / bucket_size;
  sent[e->phase]++;

  //TODO: Check if min_sent is even needed for this config 
  if( e->phase > cur_bucket) {
    if(e->ts < min_sent[cur_bucket]) {
      min_sent[cur_bucket] = e->ts;
    }
  }
  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  }
}
