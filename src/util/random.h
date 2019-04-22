#ifndef _RANDOM_H_
#define _RANDOM_H_

#include "typedefs.h"

#define tw_opi 6.28318530718

#ifdef USE_XOROSHIRO
#include "xoroshiro.h"
typedef xoroshiro128plus tw_rng_stream;
#define tw_rand_unif(G)         std::generate_canonical<double, std::numeric_limits<double>::digits>(G)
#define tw_rand_reverse_unif(G) G.prev()
#else
#include "clcg4.h"
typedef clcg4 tw_rng_stream;
#define tw_rand_unif(G)         G()
#define tw_rand_reverse_unif(G) G.prev()
#endif

/*
 * Public Function Prototypes
 */
extern void     tw_rand_init(uint32_t v, uint32_t w);

extern long     tw_rand_integer(tw_rng_stream&  g, long low, long high);
extern unsigned long tw_rand_ulong(tw_rng_stream&  g, unsigned long low, unsigned long high);
extern long     tw_rand_binomial(tw_rng_stream&  g, long N, double P);
extern double   tw_rand_exponential(tw_rng_stream&  g, double Lambda);
extern double   tw_rand_pareto(tw_rng_stream&  g, double scale, double shape);
extern double   tw_rand_gamma(tw_rng_stream&  g, double shape, double scale);
extern long     tw_rand_geometric(tw_rng_stream&  g, double P);
extern double   tw_rand_normal01(tw_rng_stream&  g, unsigned int *rng_calls);
extern double   tw_rand_normal_sd(tw_rng_stream&  g, double Mu, double Sd, unsigned int *rng_calls);
extern long     tw_rand_poisson(tw_rng_stream&  g, double Lambda);
extern double   tw_rand_weibull(tw_rng_stream&  g, double mean, double shape);

#endif
