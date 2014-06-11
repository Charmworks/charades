#include "typedefs.h"

// The LPType contains function pointers for handling/reversing events as well
// as maps on how to locate LP structs based on their global ids.
struct LPType {
  init_f init;
  event_f execute;
  revent_f reverse;
  finalize_f finalize;
  map_f global_map;
  map_f local_map;
};

// TODO: Need to flesh this out more
// Right now, an LPStruct is an LPType, as well as its state.
struct LPStruct {
  LPChare* owner;
  unsigned gid;
  void* state;
  LPType* type;
};
