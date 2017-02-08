#ifndef _GVTMANAGER_H
#define _GVTMANAGER_H

#include "gvtmanager.decl.h"
#include <float.h>

extern CProxy_GvtManager gvts;

struct GVT {
  GVT() : ts(DBL_MAX), type(0) {}
  Time ts;
  unsigned type;
};

class GvtManager : public CBase_GvtManager {
 protected:
  Time gvt;

 public:
  GvtManager();

  Time current_gvt() const { return gvt; }
  virtual void gvt_begin() {}
}; 

class GvtSync : public CBase_GvtSync {
 public:
  GvtSync();

  void gvt_begin();
  void gvt_contribute();
  void gvt_end(CkReductionMsg*);
};

#endif
