#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include "typedefs.h"
#include "scheduler.h"
#include "gvtmanager.h"

class Trigger {
  public:
    virtual void iteration_done() = 0;
    virtual void reset() = 0;
    virtual bool ready() const = 0;
};

class BoundedTrigger : public Trigger {
  private:
    TriggerPtr trigger;
    unsigned triggers_left;
  public:
    BoundedTrigger(Trigger* t, unsigned triggers) : trigger(t), triggers_left(triggers) {}
    void iteration_done() { trigger->iteration_done(); }
    void reset() { if (triggers_left > 0) triggers_left--; trigger->reset(); }
    bool ready() const { return triggers_left > 0 && trigger->ready(); }
};

class ConstTrigger : public Trigger {
  private:
    bool value;
  public:
    ConstTrigger(bool v) : value(v) {}
    void iteration_done() {}
    void reset() {}
    bool ready() const { return value; }
};

class CountTrigger : public Trigger {
  private:
    unsigned int iter_cnt, iter_max;
  public:
    CountTrigger(unsigned max) : iter_cnt(0), iter_max(max) {}
    void iteration_done() { iter_cnt++; }
    void reset() { iter_cnt = 0; }
    bool ready() const { return iter_cnt >= iter_max; }
};

// TODO: What do to on resume before GVT is computed?
class LeashTrigger : public Trigger {
  private:
    Scheduler* scheduler;
    Time leash;
  public:
    LeashTrigger(Scheduler* sched, Time l) : scheduler(sched), leash(l) {}
    void iteration_done() {}
    void reset() {}
    bool ready() const {
      return scheduler->get_min_time() > scheduler->globals->g_leash_time + leash;
    }
};

#endif
