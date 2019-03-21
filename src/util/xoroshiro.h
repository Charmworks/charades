#pragma once

#include "pup.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

class xoroshiro128plus {
private:
  static constexpr uint64_t S0 = 0x9b60939458e17d7e;
  static constexpr uint64_t S1 = 0xd737232eeccdf7ed;

  static constexpr inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
  }

  uint64_t _s[2];
  uint64_t _count = 0;

  constexpr xoroshiro128plus(uint64_t s0, uint64_t s1) noexcept
      : _s{s0, s1}, _count(0) {}

public:
  using result_type = uint64_t;

  constexpr xoroshiro128plus() noexcept : xoroshiro128plus(S0, S1) {}

  constexpr xoroshiro128plus(const xoroshiro128plus& rhs) noexcept
      : _s{rhs._s[0], rhs._s[1]}, _count(rhs._count) {}

  xoroshiro128plus(result_type r) noexcept {
    seed(r);
  }

  template <typename Sseq, typename = typename std::enable_if<!std::is_same<Sseq, xoroshiro128plus>::value>::type> 
  xoroshiro128plus(Sseq& q) {
    seed(q);
  }

  auto operator = (const xoroshiro128plus& rhs) noexcept -> xoroshiro128plus& {
    _s[0] = rhs._s[0];
    _s[1] = rhs._s[1];
    _count = rhs._count;
    return *this;
  }

  static constexpr auto min() noexcept -> result_type { return std::numeric_limits<result_type>::min(); }
  static constexpr auto max() noexcept -> result_type { return std::numeric_limits<result_type>::max(); }

  constexpr auto operator () () noexcept -> result_type {
    const uint64_t result = _s[0] + _s[1];

    const uint64_t s0 = rotl(_s[0], 24) ^ _s[0] ^ _s[1] ^ ((_s[0] ^ _s[1]) << 16);
    const uint64_t s1 = rotl(_s[0] ^ _s[1], 37);
    _s[0] = s0;
    _s[1] = s1;

    ++_count;
    return result;
  }

  constexpr void prev() noexcept {
    const uint64_t x = rotl(_s[1], 27);
    _s[0] = rotl(_s[0] ^ x ^ (x << 16), 40);
    _s[1] = _s[0] ^ x;
    --_count;
  }

  constexpr void seed() {
    _s[0] = S0;
    _s[1] = S1;
  }

  constexpr void seed(result_type r) {
    uint64_t z = r += 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    _s[0] = z ^ (z >> 31);

    z = r += 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    _s[1] = z ^ (z >> 31);
  }

  template <typename Sseq>
  typename std::enable_if<std::is_class<Sseq>::value>::type seed(Sseq& q) {
    q.generate(_s, _s + 2);
  }

  constexpr void discard(uint64_t n) noexcept {
    for (unsigned long long i = 0; i < n; ++i) {
      operator () ();
    }
  }

  void jump() noexcept {
    static const uint64_t JUMP[2] = {
      0xDF900204d8f554A5,
      0x170865DF4B3201FC
    };

    uint64_t s0 = 0;
    uint64_t s1 = 0;

    for (int b = 0; b < 64; ++b) {
      if (JUMP[0] & UINT64_C(1) << b) {
        s0 ^= _s[0];
        s1 ^= _s[1];
      }
      operator () ();
    }

    for (int b = 0; b < 64; ++b) {
      if (JUMP[1] & UINT64_C(1) << b) {
        s0 ^= _s[0];
        s1 ^= _s[1];
      }
      operator () ();
    }

    _s[0] = s0;
    _s[1] = s1;
    _count = 0;
  }

  void long_jump() noexcept {
    static const uint64_t JUMP[2] = {
      0xD2A98B26625EEE7B,
      0xDDDF9B1090AA7AC1
    };

    uint64_t s0 = 0;
    uint64_t s1 = 0;

    for (int b = 0; b < 64; ++b) {
      if (JUMP[0] & UINT64_C(1) << b) {
        s0 ^= _s[0];
        s1 ^= _s[1];
      }
      operator () ();
    }

    for (int b = 0; b < 64; ++b) {
      if (JUMP[1] & UINT64_C(1) << b) {
        s0 ^= _s[0];
        s1 ^= _s[1];
      }
      operator () ();
    }

    _s[0] = s0;
    _s[1] = s1;
    _count = 0;
  }

  constexpr uint64_t count() noexcept { return _count; }

  virtual void pup(PUP::er& p) {
    p | _s[0];
    p | _s[1];
    p | _count;
  }

  constexpr auto operator == (const xoroshiro128plus& rhs) noexcept -> bool {
    return _s[0] == rhs._s[0] && _s[1] == rhs._s[1];
  }

  constexpr auto operator != (const xoroshiro128plus& rhs) noexcept -> bool {
    return !(*this == rhs);
  }

  friend auto operator << (std::ostream& os, const xoroshiro128plus& rhs) -> std::ostream& {
    os << rhs._s[0] << ' ' << rhs._s[1];
    return os;
  }

  friend auto operator >> (std::istream& is, xoroshiro128plus& rhs) -> std::istream& {
    is >> rhs._s[0];
    is >> rhs._s[1];
    return is;
  }
};
