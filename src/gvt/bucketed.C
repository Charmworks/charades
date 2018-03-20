#include "bucketed.h"

#include "event.h"
#include "globals.h"
#include "scheduler.h"

#include <float.h>

BucketGVT::BucketGVT() {
  gvt_name = "Bucket GVT";
  active = false;
  continuous = true;

  curr_bucket = 0;
  bucket_size = g_tw_gvt_bucket_size;
  total_buckets = ceil(g_tw_ts_end / bucket_size);

  sent = new int[total_buckets];
  received = new int[total_buckets];
  offsets = new int[total_buckets];
  anti_offsets = new int[total_buckets];
  for (int i = 0; i < total_buckets; i++) {
    sent[i] = 0;
    received[i] = 0;
    offsets[i] = 0;
    anti_offsets[i] = 0;
  }
}

void BucketGVT::finalize() {
  int total_reg = 0;
  int total_anti = 0;
  int total_reg_offset = 0;
  int total_anti_offset = 0;

  for (int i = 0; i < total_buckets; i++) {
    total_reg += offsets[i];
    total_reg_offset += offsets[i] * i;
    total_anti += anti_offsets[i];
    total_anti_offset += anti_offsets[i] * i;

    if (offsets[i] > 0) {
      double percentage = ((double)anti_offsets[i] / offsets[i]) * 100;
      CkPrintf("GVT[%i:%i]: Percentage of cancelled events %f%%\n",
          CkMyPe(), i, percentage);
    }
  }
  CkPrintf("GVT[%i]: Average event offset %lf, Average anti offset %lf\n",
      CkMyPe(),
      (double)total_reg_offset / total_reg,
      (double)total_anti_offset / total_anti);
}

void BucketGVT::gvt_begin() {
  attempt_gvt();
  scheduler->gvt_resume();
}

void BucketGVT::gvt_end(int count, int* invalid) {
  active = false;

  int num_valid = 0;
  while (!invalid[num_valid] && num_valid < count) { num_valid++; }

  if (num_valid == 0) {
    attempt_gvt();
  } else {
    prev_gvt = curr_gvt;
    curr_bucket += num_valid;
    curr_gvt = curr_bucket * bucket_size;

    if (curr_gvt >= g_tw_ts_end) {
      finalize();
    }

    scheduler->gvt_done(curr_gvt);
  }
}

int BucketGVT::buckets_passed() const {
  return (int)((std::min(scheduler->get_min_time(), g_tw_ts_end) - curr_gvt) / bucket_size);
}

void BucketGVT::attempt_gvt() {
  int num_passed = buckets_passed();
  if (num_passed > 0 && !active) {
    active = true;
    contribute(sizeof(int), &num_passed, CkReduction::min_int,
        CkCallback(CkReductionTarget(BucketGVT, all_ready), thisProxy));
  }
}

void BucketGVT::all_ready(int num_buckets) {
  send_counts(num_buckets);
}

void BucketGVT::send_counts(int num_buckets) {
  int* data = new int[3 * num_buckets];
  int num_passed = buckets_passed();

  for (int i = 0; i < num_buckets; i++) {
    data[i*3] = sent[curr_bucket + i];
    data[i*3+1] = received[curr_bucket + i];
    data[i*3+2] = num_passed < i + 1 ? 1 : 0;
  }

  contribute(3 * num_buckets * sizeof(int), data, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT, check_counts), thisProxy));

  delete[] data;
}

/**
 * Check all counts sent and see if any match and are therefore completed. If
 * any have completed then contribute to gvt_end. Otherwise, recheck counts for
 * all that are still valid.
 */
void BucketGVT::check_counts(int count, int* data) {
  int num_buckets = count / 3;
  int num_valid = 0;
  bool completed = false;
  while (!data[num_valid * 3 + 2] && num_valid < num_buckets) {
    int sent = data[num_valid*3];
    int recvd = data[num_valid*3+1];
    if (sent != recvd && completed) {
      break;
    } else if (sent == recvd) {
      completed = true;
    }
    num_valid++;
  }
  if (num_valid == 0) {
    active = false;
    attempt_gvt();
  } else if (!completed) {
    send_counts(num_valid);
  } else {
    int* invalid = new int[num_valid];
    int num_passed = buckets_passed();
    for (int i = 0; i < num_valid; i++) {
      invalid[i] = num_passed < i + 1 ? 1 : 0;
    }
    contribute(num_valid * sizeof(int), invalid, CkReduction::sum_int,
      CkCallback(CkReductionTarget(BucketGVT, gvt_end), thisProxy));

    delete[] invalid;
  }
}

void BucketGVT::consume(RemoteEvent* e) {
  received[e->phase]++;
  attempt_gvt();
}

void BucketGVT::produce(RemoteEvent* e) {
  e->phase = e->ts / bucket_size;

  int offset = e->phase - curr_bucket;
  if (e->anti) {
    anti_offsets[offset]++;
  } else {
    offsets[offset]++;
  }

  sent[e->phase]++;
  attempt_gvt();
}
