#ifndef ROSS_SETUP_H_
#define ROSS_SETUP_H_

#include "typedefs.h"

void tw_init(int argc, char** argv);
void tw_create_simulation(LPFactory* factory, LPMapper* mapper);
void tw_create_simulation(LPFactory* factory);
void tw_run();
void tw_end();

class LPFactory {
  public:
    virtual LPBase* create_lp(uint64_t gid) const = 0;
};

class LPMapper {
  public:
    virtual uint64_t get_chare_id(uint64_t global_id) const = 0;
    virtual uint64_t get_local_id(uint64_t global_id) const = 0;
    virtual uint64_t get_global_id(uint64_t chare_id, uint64_t local_id) const = 0;
    virtual uint64_t get_num_lps(uint64_t chare_id) const = 0;
};

#endif
