#ifndef _GVTMANAGER_H
#define _GVTMANAGER_H

#include "gvtmanager.decl.h"
#include "typedefs.h"
#include "globals.h"
#include "statistics.h"

#include "pe.h"
#include "pe_queue.h"


class GvtManager : public CBase_GvtManager {

  public:
    GvtManager();
    GvtManager(CProxy_Initialize);

    virtual void gvt_begin();

  private:

}; 

class GvtSynch : public CBase_GvtSynch {

  public:
    void gvt_begin();
    void gvt_contribute();
    void gvt_end(CkReductionMsg*);

    GvtSynch(CProxy_Initialize);
  private:
    

};
#endif

