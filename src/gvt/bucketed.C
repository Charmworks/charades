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

  num_buckets = ceil(g_tw_ts_end / bucket_size);
  curr_bucket = 0;
  sent = new int[num_buckets];
  received = new int[num_buckets];
  for (int i = 0; i < num_buckets; i++) {
    sent[i] = 0;
    received[i] = 0;
  }
}

void BucketGVT::gvt_begin() {
  attempt_gvt();
  scheduler->gvt_resume();
}

void BucketGVT::gvt_end(int invalid) {
  active = false;
  if (invalid) {
    attempt_gvt();
  } else {
    prev_gvt = curr_gvt;
    curr_gvt = ++curr_bucket * bucket_size;
    scheduler->gvt_done(curr_gvt);
  }
}

int BucketGVT::passed_bucket() const {
  if (scheduler->get_min_time() >= (curr_bucket + 1) * bucket_size) {
    return 1;
  } else {
    return 0;
  }
}

void BucketGVT::attempt_gvt() {
  if (passed_bucket() && !active) {
    active = true;
    contribute(CkCallback(CkReductionTarget(BucketGVT, all_ready), thisProxy));
  }
}

void BucketGVT::all_ready() {
  send_counts();
}

void BucketGVT::send_counts() {
  int data[3] = { sent[curr_bucket], received[curr_bucket], !passed_bucket() };
  contribute(3 * sizeof(int), data, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT, check_counts), thisProxy));
}

void BucketGVT::check_counts(int sent, int recvd, int invalid) {
  if (invalid) {
    active = false;
    attempt_gvt();
  } else if (sent != recvd) {
    send_counts();
  } else {
    int invalid = !passed_bucket();
    contribute(sizeof(int), &invalid, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT, gvt_end), thisProxy));
  }
}


void BucketGVT::consume(RemoteEvent* e) {
  received[e->phase]++;
  attempt_gvt();
}

void BucketGVT::produce(RemoteEvent* e) {
  e->phase = e->ts / bucket_size;
  sent[e->phase]++;
  attempt_gvt();
}
