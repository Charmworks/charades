#include "ross_block.h"

#include "globals.h"

// TODO: Map verification code

// Takes a chare index and the local id of an LP on that chare, and returns
// the global id for that chare. Has to be consistent with the local and
// chare maps defined below.
uint64_t init_block_map(uint64_t chare, uint64_t local_id) {
  return g_lps_per_chare*chare + local_id;
}

// Given a global lp id, this map returns the index of the LP chare that holds
// the desired LP.
uint64_t chare_block_map(uint64_t global_id) {
  return global_id / g_lps_per_chare;
}

// Given a global lp id, this map returns the local id of the desired LP.
uint64_t local_block_map(uint64_t global_id) {
  return global_id % g_lps_per_chare;
}

// Constant number of lps per chare, as defined by the global
uint64_t constant_numlp_map(uint64_t chare) {
  return g_lps_per_chare;
}
