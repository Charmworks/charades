#ifndef _LP_STRUCTS_H
#define _LP_STRUCTS_H

#include "typedefs.h"

// The LPType contains function pointers for handling/reversing events as well
// as maps on how to locate LP structs based on their global ids.
struct LPType {
  init_f init;
  event_f execute;
  revent_f reverse;
  final_f finalize;
  commit_f commit;
  size_t state_size;
};

class LP;
struct tw_rng_stream;
// Right now, an LPStruct is an LPType, as well as its state.
struct LPStruct {
  LP* owner;
  unsigned gid;
  void* state;
  LPType* type;
  tw_rng_stream* rng;
};
#endif
