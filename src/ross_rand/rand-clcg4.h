#ifndef INC_clcg4_h
#define INC_clcg4_h

// TODO: Better includes
#include <stdio.h>
#include <stdint.h>


enum SeedType
{
	InitialSeed, LastSeed, NewSeed
};

typedef int32_t * tw_seed;

typedef enum SeedType SeedType;

// TODO: This is temporary
void tw_error(const char *file, int line, const char *fmt,...);
void* tw_calloc(const char* file, int line, const char* for_who, size_t e_sz, size_t n);
size_t g_tw_rng_max = 1;
tw_seed* g_tw_rng_seed = NULL;

struct tw_rng
{
	/*
	 * equals a[i]^{m[i]-2} mod m[i]
	 */
	long long	b[4];

	/*
	 * a[j]^{2^w} et a[j]^{2^{v+w}}.
	 */
	int32_t	m[4];
	int32_t	a[4];
	int32_t	aw[4];
	int32_t	avw[4];

	// the seed..
	int32_t	seed[4];
};

struct tw_rng_stream
{
	int32_t	 Ig[4];
	int32_t	 Lg[4];
	int32_t	 Cg[4];

	//tw_rng	*rng;

#ifdef RAND_NORMAL
	double	 tw_normal_u1;
	double	 tw_normal_u2;
	int	 tw_normal_flipflop;
#endif
};

extern tw_rng	*rng_init(int v, int w);
extern void     rng_set_initial_seed();
extern void     rng_init_generator(tw_rng_stream * g, SeedType Where);
extern void     rng_set_seed(tw_rng_stream * g, uint32_t * s);
extern void     rng_get_state(tw_rng_stream * g, uint32_t * s);
extern void     rng_write_state(tw_rng_stream * g, FILE *f);
extern double   rng_gen_val(tw_rng_stream * g);
extern double   rng_gen_reverse_val(tw_rng_stream * g);

#endif
