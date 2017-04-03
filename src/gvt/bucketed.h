#ifndef _BUCKETED_H_
#define _BUCKETED_H_

#include "gvtmanager.h"

class BucketGVT : public CBase_BucketGVT {
  public:

    BucketGVT();
    /** Switch phases if next phase ready and start GVT process for current phase**/
    void gvt_begin();
    /**Check if phase has completed detection, if so contribute min time to all reduce**/
    void check_counts(int, int, int);

    void bucket_ready();
    /** Called by the all reduce from check_counts() with resulting gvt**/
    void gvt_end(Time, double);

    /** initialize arrays and phases for detection **/
    void initialize_buckets();

    /**Increment received count for the phase of the event **/
    void consume(RemoteEvent* e);
    /**Increment sent count for producing phase and recalculate min_sent**/
    void produce(RemoteEvent* e);

  private:

    /**start and end phase of the gvt**/
    unsigned bucket_size, cur_bucket;
    bool doing_reduction;
    int * rollback_flags;
    int * sent;
    int * received;
    Time*  min_sent;
};

#endif
