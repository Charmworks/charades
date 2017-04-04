#ifndef _BUCKETED_H_
#define _BUCKETED_H_

#include "gvtmanager.h"

class BucketGVT : public CBase_BucketGVT {
  public:

    BucketGVT();
    /** Check if program has crossed bucket boundary**/
    void gvt_begin();
    /**Check if sent = recieved for bucket and that no bucket rolled back**/
    void check_counts(int, int, int);
    /**Serves as checkpoint to ensure all PES cross boundary before we start reducing counts **/
    void bucket_ready();
    /** Called by the all reduce from check_counts() with resulting gvt**/
    void gvt_end(Time, double);
    /**Increment received count for the proper bucket of the event, check boundaries **/
    void consume(RemoteEvent* e);
    /**Increment sent count for proper bucket and recalculate min_sent, check boundaries**/
    void produce(RemoteEvent* e);

  private:

    /**start and end phase of the gvt**/
    unsigned bucket_size, cur_bucket;
    /** indicates if cur bucket is doing reductions **/
    bool doing_reduction;
    int * rollback_flags;
    int * sent;
    int * received;
    Time*  min_sent;
};

#endif
