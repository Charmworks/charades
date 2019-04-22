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
void     tw_rand_init(uint32_t v, uint32_t w);

int64_t  tw_rand_integer(tw_rng_stream& g, int64_t low, int64_t high);
uint64_t tw_rand_ulong(tw_rng_stream& g, uint64_t low, uint64_t high);
int64_t  tw_rand_binomial(tw_rng_stream& g, int64_t N, double P);
double   tw_rand_exponential(tw_rng_stream& g, double Lambda);
double   tw_rand_pareto(tw_rng_stream& g, double scale, double shape);
double   tw_rand_gamma(tw_rng_stream& g, double shape, double scale);
int64_t  tw_rand_geometric(tw_rng_stream& g, double P);
double   tw_rand_normal01(tw_rng_stream& g);
double   tw_rand_normal_sd(tw_rng_stream& g, double Mu, double Sd);
double   tw_rand_lognormal(tw_rng_stream& g, double mean, double sd);
int64_t  tw_rand_poisson(tw_rng_stream& g, double Lambda);
double   tw_rand_weibull(tw_rng_stream& g, double mean, double shape);

#endif
