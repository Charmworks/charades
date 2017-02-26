#ifndef _PE_H
#define _PE_H

#include "pe.decl.h"

CkReductionMsg *statsReduction(int nMsg, CkReductionMsg **msgs);
extern CkReduction::reducerType statsReductionType;

struct MemUsage {
  unsigned long long max_memory;
  double avg_memory;
};

#endif
