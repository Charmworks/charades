#ifndef FACTORY_H_
#define FACTORY_H_

/**
 * Base class which defines the factory interface used during simulation setup.
 */
class LPFactory {
  public:
    /** Creates the LP associated with global id \p gid */
    virtual LPBase* create_lp(uint64_t gid) const = 0;
};

#endif
