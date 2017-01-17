#ifndef _GVT_MANAGER_H
#define _GVT_MANAGER_H

#include "gvt_manager.decl.h"
#include "typedefs.h"
#include "globals.h"
#include "statistics.h"
//#include "lp.h" // Included for LPToken definition

#include "pe_queue.h"


struct GVT {
  GVT() : ts(DBL_MAX), type(0) {}
  Time ts;
  unsigned type;
};

class GVT_Manager : public CBase_GVT_Manager {

  public:

    virtual void gvt_begin() = 0;

    //virtual void gvt_print(GVT* gvt_struct);

    virtual bool wait_on_gvt();

  private:

}; 

class GVT_Synch : public GVT_Manager {

  public:
    void gvt_begin();
    bool wait_on_gvt();

    GVT_Synch();
  private:
    void gvt_contribute();
    void gvt_end();

    bool waiting_on_gvt;
    bool gvt_started;
};

#endif

