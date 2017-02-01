#ifndef _GVTMANAGER_H
#define _GVTMANAGER_H

#include "gvtmanager.decl.h"
//#include "typedefs.h"
//#include "globals.h"
//#include "statistics.h"

//#include "pe.h"
//#include "pe_queue.h"


class GvtManager : public CBase_GvtManager {

  public:
    GvtManager();
    GvtManager(CProxy_Initialize);

    virtual void gvt_begin();

  private:

}; 

class GvtSync : public CBase_GvtSync {

  public:
    void gvt_begin();
    void gvt_contribute();
    void gvt_end(CkReductionMsg*);

    GvtSync();
    GvtSync(CProxy_Initialize);
  private:
    

};

#endif

