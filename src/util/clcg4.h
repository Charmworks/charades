#ifndef _CLCG4_H_
#define _CLCG4_H_

#include "pup.h"

#include <cstdint>
#include <iostream>

/**
 * Implementation of a CLCG4 random number generator, based on the reversible
 * implementation from ROSS (https://github.com/ROSS-org/ROSS)
 */

class clcg4 {
private:
  enum SeedType {
    InitialSeed, LastSeed, NewSeed
  };

  struct tw_rng {
    int32_t m[4];
    int32_t a[4];
    int32_t aw[4];
    int32_t avw[4];

    int64_t b[4]; // Used to make the RNG reversible

    int32_t seed[4];
  };
  static tw_rng rng; // One RNG per process, once intialized, it doesn't change

  int32_t Ig[4];
  int32_t Lg[4];
  int32_t Cg[4];

  uint64_t _count = 0;

#ifdef RAND_NORMAL
  double tw_normal_u1;
  double tw_normal_u2;
  int    tw_normal_flipflop;
#endif

  static int64_t find_b(int64_t a, int64_t k, int64_t m) {
    int64_t sqrs[32];
    sqrs[0] = a;
    for (int i = 1; i < 32; i++) {
      sqrs[i] =(sqrs[i - 1] * sqrs[i - 1]) % m;
    }

    int64_t power_of_2 = 1;
    int64_t b = 1;
    for (int i = 0; i < 32; i++) {
      if (!(power_of_2 & k)) {
        sqrs[i] = 1;
      }
      b =(b * sqrs[i]) % m;
      power_of_2 = power_of_2 * 2;
    }

    return b;
  }

  static constexpr int32_t H = 32768;
  static int32_t mult_mod_M(int32_t s, int32_t t, int32_t M) {
    int32_t R, S0, S1, q, qh, rh, k;

    if (s < 0) { s += M; }
    if (t < 0) { t += M; }

    if (s < H) {
      S0 = s;
      R = 0;
    } else {
      S1 = s / H;
      S0 = s - H * S1;
      qh = M / H;
      rh = M - H * qh;

      if (S1 >= H) {
        S1 -= H;
        k = t / qh;
        R = H *(t - k * qh) - k * rh;
        while(R < 0) { R += M; }
      } else {
        R = 0;
      }

      if (S1 != 0) {
        q = M / S1;
        k = t / q;
        R -= k *(M - S1 * q);
        if (R > 0) { R -= M; }
        R += S1 *(t - k * q);
        while(R < 0) { R += M; }
      }
      k = R / qh;
      R = H *(R - k * qh) - k * rh;
      while(R < 0) { R += M; }
    }

    if (S0 != 0) {
      q = M / S0;
      k = t / q;
      R -= k *(M - S0 * q);
      if (R > 0) { R -= M; }
      R += S0 *(t - k * q);
      while(R < 0) { R += M; }
    }

    return R;
  }

  void init_generator(SeedType Where) {
    for (int j = 0; j < 4; j++) {
      switch(Where) {
        case InitialSeed:
          Lg[j] = Ig[j];
          break;
        case NewSeed:
          Lg[j] = mult_mod_M(rng.aw[j], Lg[j], rng.m[j]);
          break;
        case LastSeed:
          break;
      }
      Cg[j] = Lg[j];
    }
  }

public:
  using result_type = double;

  static void init(int v, int w) {
    int32_t default_seed[4] = {11111111, 22222222, 33333333, 44444444};
    init(v, w, default_seed);
  }

  static void init(int v, int w, int32_t init_seed[4]) {
    // Init m
    rng.m[0] = 2147483647;
    rng.m[1] = 2147483543;
    rng.m[2] = 2147483423;
    rng.m[3] = 2147483323;

    // Init a
    rng.a[0] = 45991;
    rng.a[1] = 207707;
    rng.a[2] = 138556;
    rng.a[3] = 49689;

    for (int j = 0; j < 4; j++) {
      // Init aw
      rng.aw[j] = rng.a[j];
      for (int i = 0; i < w; i++) {
        rng.aw[j] = mult_mod_M(rng.aw[j], rng.aw[j], rng.m[j]);
      }

      // Init avw
      rng.avw[j] = rng.aw[j];
      for (int i = 0; i < v; i++) {
        rng.avw[j] = mult_mod_M(rng.avw[j], rng.avw[j], rng.m[j]);
      }

      // Init b
      rng.b[j] = find_b(rng.a[j], rng.m[j] - 2, rng.m[j]);

      // Init seed
      rng.seed[j] = init_seed[j];
    }
  }

  clcg4() { seed(); }
  clcg4(int32_t s[4]) { seed(s); }
  clcg4(uint64_t id) { seed(id); }

  void seed() {
    seed(42);
  }

  void seed(int32_t s[4]) {
    for (int j = 0; j < 4; j++)
      Ig[j] = s[j];

    init_generator(InitialSeed);
  }

  void seed(uint64_t id) {
    uint64_t mask_bit = 1;

    int32_t Ig_t[4];
    int32_t avw_t[4];

    int positions = ((sizeof(uint64_t)) * 8) - 1;

    //seed for zero
    for (int j = 0; j < 4; j++) {
      Ig_t[j] = rng.seed[j];
    }

    mask_bit <<= positions;

    do {
      if (id & mask_bit) {
        for (int j = 0; j < 4; j++) {
          avw_t[j] = rng.avw[j];

          // exponentiate modulus
          for (int i = 0; i < positions; i++) {
            avw_t[j] = mult_mod_M(avw_t[j], avw_t[j], rng.m[j]);
          }

          Ig_t[j] = mult_mod_M(avw_t[j], Ig_t[j], rng.m[j]);
        }
      }

      mask_bit >>= 1;
      positions--;
    } while(positions > 0);

    if (id % 2) {
      for (int j = 0; j < 4; j++) {
        Ig_t[j] = mult_mod_M(rng.avw[j], Ig_t[j], rng.m[j]);
      }
    }

    for (int j = 0; j < 4; j++) {
      Ig[j] = Ig_t[j];
    }

    init_generator(InitialSeed);
  }

  static constexpr result_type min() noexcept { return 0.0; }
  static constexpr result_type max() noexcept { return 1.0; }

  constexpr result_type operator() () noexcept {
    int32_t s = Cg[0];
    int32_t k = s / 46693;
    double u = 0.0;

    s = 45991 *(s - k * 46693) - k * 25884;
    if (s < 0)
      s = s + 2147483647;
    Cg[0] = s;
    u = u + 4.65661287524579692e-10 * s;

    s = Cg[1];
    k = s / 10339;
    s = 207707 *(s - k * 10339) - k * 870;
    if (s < 0)
      s = s + 2147483543;
    Cg[1] = s;
    u = u - 4.65661310075985993e-10 * s;
    if (u < 0)
      u = u + 1.0;

    s = Cg[2];
    k = s / 15499;
    s = 138556 *(s - k * 15499) - k * 3979;
    if (s < 0.0)
      s = s + 2147483423;
    Cg[2] = s;
    u = u + 4.65661336096842131e-10 * s;
    if (u >= 1.0)
      u = u - 1.0;

    s = Cg[3];
    k = s / 43218;
    s = 49689 *(s - k * 43218) - k * 24121;
    if (s < 0)
      s = s + 2147483323;
    Cg[3] = s;
    u = u - 4.65661357780891134e-10 * s;
    if (u < 0)
      u = u + 1.0;

    _count++;
    return u;
  }

  void prev() noexcept {
    double u = 0.0;

    int32_t s = Cg[0];
    s = (rng.b[0] * s) % rng.m[0];
    Cg[0] = s;
    u = u + 4.65661287524579692e-10 * s;

    s = Cg[1];
    s = (rng.b[1] * s) % rng.m[1];
    Cg[1] = s;
    u = u - 4.65661310075985993e-10 * s;
    if (u < 0) { u = u + 1.0; }

    s = Cg[2];
    s = (rng.b[2] * s) % rng.m[2];
    Cg[2] = s;
    u = u + 4.65661336096842131e-10 * s;
    if (u >= 1.0) { u = u - 1.0; }

    s = Cg[3];
    s = (rng.b[3] * s) % rng.m[3];
    Cg[3] = s;
    u = u - 4.65661357780891134e-10 * s;
    if (u < 0) { u = u + 1.0; }

    _count--;
  }

  constexpr uint64_t count() noexcept { return _count; }

  void pup(PUP::er& p) {
    PUParray(p, Ig, 4);
    PUParray(p, Lg, 4);
    PUParray(p, Cg, 4);

#ifdef RAND_NORMAL
    p | tw_normal_u1;
    p | tw_normal_u2;
    p | tw_normal_flipflop;
#endif
  }

  constexpr void discard(uint64_t n) noexcept {
    for (uint64_t i = 0; i < n; ++i) {
      operator () ();
    }
  }

  constexpr bool operator == (const clcg4& rhs) noexcept {
    return Ig[0] == rhs.Ig[0] && Ig[1] == rhs.Ig[1] && Ig[2] == rhs.Ig[2] && Ig[3] == rhs.Ig[3] &&
        Lg[0] == rhs.Lg[0] && Lg[1] == rhs.Lg[1] && Lg[2] == rhs.Lg[2] && Lg[3] == rhs.Lg[3] &&
        Cg[0] == rhs.Cg[0] && Cg[1] == rhs.Cg[1] && Cg[2] == rhs.Cg[2] && Cg[3] == rhs.Cg[3];
  }

  constexpr bool operator != (const clcg4& rhs) noexcept {
    return !(*this == rhs);
  }

  friend std::ostream& operator << (std::ostream& os, const clcg4& rhs) {
    os << rhs.Ig[0] << ' ' << rhs.Ig[1] << ' ' << rhs.Ig[2] << ' ' << rhs.Ig[3] << ' ';
    os << rhs.Lg[0] << ' ' << rhs.Lg[1] << ' ' << rhs.Lg[2] << ' ' << rhs.Lg[3] << ' ';
    os << rhs.Cg[0] << ' ' << rhs.Cg[1] << ' ' << rhs.Cg[2] << ' ' << rhs.Cg[3];
    return os;
  }

  friend auto operator >> (std::istream& is, clcg4& rhs) -> std::istream& {
    is >> rhs.Ig[0];
    is >> rhs.Ig[1];
    is >> rhs.Ig[2];
    is >> rhs.Ig[3];
    is >> rhs.Lg[0];
    is >> rhs.Lg[1];
    is >> rhs.Lg[2];
    is >> rhs.Lg[3];
    is >> rhs.Cg[0];
    is >> rhs.Cg[1];
    is >> rhs.Cg[2];
    is >> rhs.Cg[3];
    return is;
  }
};

#endif
