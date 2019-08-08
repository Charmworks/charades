#ifndef _COMPLETION_H_
#define _COMPLETION_H_

#include "gvtmanager.h"

class CompletionDetector;
class CProxy_CompletionDetector;

class CdGVT : public CBase_CdGVT {
  public:
    CdGVT();

    /** Methods for initialization of completion detectors */
    void initialize_detectors();
    void broadcast_detector_proxies(int, CProxy_CompletionDetector*);

    void gvt_begin();       /**< Switches GVT phase (if possible) */
    void gvt_contribute();  /**< Contributes min time after completion */
    void gvt_end(Time);     /**< Result of min time reduction */

    void consume(RemoteEvent* e); /**< Decrement event count */
    bool produce(RemoteEvent* e); /**< Increment event count */

  private:
    unsigned max_phase, current_phase, next_phase;
    bool* detector_ready;
    CProxy_CompletionDetector* detector_proxies;
    CompletionDetector** detector_pointers;

    Time min_sent;
};

#endif
