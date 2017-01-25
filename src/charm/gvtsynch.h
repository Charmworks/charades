#ifndef _GVTSYNCH_H
#define _GVTSYNCH_H

#include "gvtsynch.decl.h"
#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
//#include "lp.h" // Included for LPToken definition
#include "pe.h"
#include "pe_queue.h"

/*
class GVT_Manager : public CBase_GVT_Manager {

  public:

    virtual void gvt_begin() = 0;

    //virtual void gvt_print(GVT* gvt_struct);

    //virtual bool wait_on_gvt();

  private:

}; 
*/

class GvtSynch: public CBase_GvtSynch {
//public GVT_Manager {

  public:
    void gvt_begin();
    void gvt_contribute();
    void gvt_end(CkReductionMsg*);
    //bool wait_on_gvt();

    GvtSynch(CProxy_Initialize);
  private:
    

    //bool waiting_on_gvt;
    bool gvt_started;
};
/*
struct GVT {
  GVT() : ts(DBL_MAX), type(0) {}
  Time ts;
  unsigned type;
};
*/
#endif

