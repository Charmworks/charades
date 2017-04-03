#include "bucketed.h"

#include "charm_functions.h"
#include "event.h"
#include "globals.h"
#include "scheduler.h"

#include <float.h>

BucketGVT::BucketGVT() {
  gvt_name = "Bucket GVT";
  initialize_buckets();

}

void BucketGVT::initialize_buckets() {
  //TODO: Make this input argument
  bucket_size = 8;

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

void BucketGVT::gvt_begin() {
   DEBUG_PE("Gvt Begin\n");

  if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size && !doing_reduction) {
    rollback_flags[cur_bucket] = 0;
    doing_reduction = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
  }
  scheduler->gvt_resume();

}

void BucketGVT::bucket_ready() {

   DEBUG_PE("bucket ready for %d \n", cur_bucket);
  int data[3] = {sent[cur_bucket], received[cur_bucket], rollback_flags[cur_bucket]};

  contribute(3 * sizeof(int), data, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT,check_counts),thisProxy));

}

void BucketGVT::check_counts(int sent, int recvd, int flag ) {

  DEBUG_PE("Checking Counts\n");
  if(flag != 0) {

   DEBUG_PE("rollback detected\n");
    doing_reduction = false;
    if (scheduler->get_min_time() >= (1 + cur_bucket) * bucket_size) {
      rollback_flags[cur_bucket] = 0;
      doing_reduction = true;
      contribute(CkCallback(CkReductionTarget(BucketGVT, bucket_ready), thisProxy));
    }
  }
  else if (sent != recvd) {

   DEBUG_PE("Counts don't match\n");
    bucket_ready();
  }
  else {

    Time min_time = scheduler->get_min_time();
    min_time = fmin(min_time, min_sent[cur_bucket]);
    CkAssert(min_time >= curr_gvt);
    double flag = 1;
    if (rollback_flags[cur_bucket]) flag = 0;

    double data[2] = {min_time, flag};

    contribute(2 * sizeof(double), data, CkReduction::min_double,
      CkCallback(CkReductionTarget(BucketGVT,gvt_end),thisProxy));
  }
}

void BucketGVT::gvt_end(Time new_gvt, double flag) {
DEBUG_PE("GVT_END\n");
  if (flag == 0) {
DEBUG_PE("END: rollback detected\n");
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
    DEBUG_PE("Gvt End for %d \n", cur_bucket);
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

  //CHECK MIN_SENT

  e->phase = e->ts / bucket_size;
  sent[e->phase]++;

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
