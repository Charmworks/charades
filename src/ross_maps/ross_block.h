#ifndef ROSS_BLOCK_H_
#define ROSS_BLOCK_H_
// Defines the mapping functions for a block mapping in ross

// TODO: Map verification code

// Takes a chare index and the local id of an LP on that chare, and returns
// the global id for that chare. Has to be consistent with the local and
// chare maps defined below.
tw_lpid init_block_map(unsigned chare, tw_lpid local_id) {
  return PE_VALUE(g_lps_per_chare)*chare + local_id;
}

// Given a global lp id, this map returns the index of the LP chare that holds
// the desired LP.
unsigned chare_block_map(tw_lpid global_id) {
  return global_id / PE_VALUE(g_lps_per_chare);
}

// Given a global lp id, this map returns the local id of the desired LP.
tw_lpid local_block_map(tw_lpid global_id) {
  return global_id % PE_VALUE(g_lps_per_chare);
}

#endif
