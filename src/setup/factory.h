#ifndef FACTORY_H_
#define FACTORY_H_

class LPFactory {
  public:
    virtual LPBase* create_lp(uint64_t gid) const = 0;
};

#endif
