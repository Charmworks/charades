#ifndef _CONSERVATIVE_H_
#define _CONSERVATIVE_H_

#include "scheduler.h"

class ConservativeScheduler : public CBase_ConservativeScheduler {
  public:
    ConservativeScheduler();
    void execute();
};

#endif
