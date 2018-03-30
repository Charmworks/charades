#include "bucketed.h"

#include "event.h"
#include "globals.h"
#include "scheduler.h"
#include "statistics.h"

#include <float.h>
#include <fstream>
#include <iomanip>

BucketGVT::BucketGVT() {
  gvt_name = "Bucket GVT";
  active = false;
  continuous = true;

  curr_bucket = 0;
  bucket_size = g_tw_gvt_bucket_size;
  total_buckets = ceil(g_tw_ts_end / bucket_size);

  reserve_threshold = g_tw_reserve_threshold;
  if (g_tw_reserve_buckets != 0) {
    reserve_buckets = g_tw_reserve_buckets;
  } else {
    reserve_buckets = total_buckets;
  }

  sent = new int[total_buckets];
  received = new int[total_buckets];
  for (int i = 0; i < total_buckets; i++) {
    sent[i] = 0;
    received[i] = 0;
  }

  if (g_tw_adaptive_buckets) {
    stats = new OffsetStats[total_buckets];
    reserves = new std::vector<RemoteEvent*>[total_buckets];
  }
}

void BucketGVT::finalize() {
  std::ofstream outfile;
  std::string filename = "gvt_" + std::to_string(CkMyPe()) + ".out";
  outfile.open(filename);
  outfile << std::fixed << std::setprecision(2);
  if (g_tw_adaptive_buckets) {
    OffsetStats total;
    OffsetStats weighted;

    for (int i = 0; i < total_buckets; i++) {
      total.regular += stats[i].regular;
      total.anti += stats[i].anti;
      total.held += stats[i].held;
      total.cancelled += stats[i].cancelled;
      total.released += stats[i].released;
      total.lag += stats[i].lag;

      weighted.regular += stats[i].regular * i;
      weighted.anti += stats[i].anti * i;

      if (stats[i].regular) {
        outfile << "==== Offset " << i << " ========" << std::endl;

        outfile << " Regular Events: " << stats[i].regular
                << " Anti Events: " << stats[i].anti
                << " (" << stats[i].anti_ratio() * 100 << "%)" << std::endl;

        outfile << " Held: " << stats[i].held
                << " (" << stats[i].held_ratio() * 100 << "%)" << std::endl;

        outfile << " Cancelled: " << stats[i].cancelled
                << " (" << stats[i].cancelled_ratio() * 100 << "%)" << std::endl;

        outfile << " Released: " << stats[i].released
                << " (" << stats[i].released_ratio() * 100 << "%)" << std::endl;

        outfile << " Average lag: " << stats[i].average_lag() << std::endl << std::endl;
      }
    }

    outfile << "==== TOTALS ========" << std::endl;
    outfile << " Regular Events: " << total.regular
            << " Anti Events: " << total.anti
            << " (" << total.anti_ratio() * 100 << "%)" << std::endl;

    outfile << " Held: " << total.held
            << " (" << total.held_ratio() * 100 << "%)" << std::endl;

    outfile << " Cancelled: " << total.cancelled
            << " (" << total.cancelled_ratio() * 100 << "%)" << std::endl;

    outfile << " Released: " << total.released
            << " (" << total.released_ratio() * 100 << "%)" << std::endl;
    outfile << " Average lag: " << total.average_lag() << std::endl << std::endl;

    outfile << "Average event offset: " << (double)weighted.regular / total.regular << std::endl;
    outfile << "Average anti offset: " << (double)weighted.anti / total.anti << std::endl;
  }
  outfile.close();
}

bool key_matches(RemoteEvent* e1, RemoteEvent* e2) {
  return e1->send_pe == e2->send_pe && e1->event_id == e2->event_id;
}

bool BucketGVT::attempt_cancel(RemoteEvent* anti) {
  std::vector<RemoteEvent*>& bucket = reserves[anti->phase];
  auto iter = bucket.begin();
  while (iter != bucket.end()) {
    if (key_matches(anti, *iter)) break;
    iter++;
  }
  if (iter != bucket.end()) {
    stats[anti->offset].cancelled++;
    RemoteEvent* original = *iter;
    bucket.erase(iter);
    delete original;
    delete anti;
    return true;
  } else {
    return false;
  }
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

    if (g_tw_adaptive_buckets && curr_bucket < total_buckets) {
      for (RemoteEvent* e : reserves[curr_bucket]) {
        stats[e->offset].released++;
        charm_event_release(e);
      }
      reserves[curr_bucket].clear();
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

bool BucketGVT::produce(RemoteEvent* e) {
  e->phase = e->ts / bucket_size;
  sent[e->phase]++;
  attempt_gvt();

  if (g_tw_adaptive_buckets) {
    if (e->anti) {
      int phase = e->phase;
      stats[e->offset].anti++;
      stats[e->offset].lag += e->offset - (e->phase - curr_bucket);
      if (attempt_cancel(e)) {
        PE_STATS(remote_cancels)++;
        PE_STATS(anti_cancels)++;
        sent[phase] -= 2;
        return false;
      } else {
        return true;
      }
    } else {
      e->offset = e->phase - curr_bucket;
      stats[e->offset].regular++;
      if (e->offset > 0 &&
          (e->offset > reserve_buckets ||
          stats[e->offset].anti_ratio() > reserve_threshold)) {
        reserves[e->phase].push_back(e);
        stats[e->offset].held++;
        PE_STATS(remote_holds)++;
        return false;
      } else {
        return true;
      }
    }
  } else {
    return true;
  }
}
