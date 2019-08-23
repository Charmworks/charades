#ifndef MAPPER_H_
#define MAPPER_H_

/**
 * Base class which defines the mapper interface used during simulation setup.
 */
class LPMapper {
  public:
    /** Maps a global LP id, \p gid, to a chare id */
    virtual uint64_t get_chare_id(uint64_t global_id) const = 0;
    /** Maps a global LP id, \p gid, to its local offset id within its chare */
    virtual uint64_t get_local_id(uint64_t global_id) const = 0;
    /** Maps a chare id and offset within the chare to a global LP id.
     * \note This is the inverse of get_chare_id and get_local_id
     */
    virtual uint64_t get_global_id(uint64_t chare_id, uint64_t local_id) const = 0;
    /** Maps a chare id to the number of LPs that chare will have */
    virtual uint64_t get_num_lps(uint64_t chare_id) const = 0;
};

/**
 * Concrete implementation of a basic block mapping to be used as a default.
 * Maps g_lps_per_chare lps to each chare in contiguous blocks of global ids.
 */
class BlockMapper : public LPMapper {
  /** The chare id where \p gid is located is \p gid / \ref g_lps_per_chare */
  uint64_t get_chare_id(uint64_t global_id) const {;
    return global_id / g_lps_per_chare;
  }
  /** The local offset of \p gid is \p gid % \ref g_lps_per_chare */
  uint64_t get_local_id(uint64_t global_id) const {
    return global_id % g_lps_per_chare;
  }
  /**
   * The global id for (\p chare_id,\ p local_id) is
   * \ref g_lps_per_chare * \p chare_id + \p local_id
   */
  uint64_t get_global_id(uint64_t chare_id, uint64_t local_id) const {
    return g_lps_per_chare * chare_id + local_id;
  }
  /** The number of lps on all chares is \ref g_lps_per_chare */
  uint64_t get_num_lps(uint64_t chare_id) const {
    return g_lps_per_chare;
  }
};

#endif
