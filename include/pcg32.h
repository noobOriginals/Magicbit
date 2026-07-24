#ifndef PCG32_H
#define PCG32_H

// Std includes
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t state, inc;
} PCG32;

PCG32* createPCG32();
void destroyPCG32(PCG32* rng);
uint64_t pcgHash(uint64_t x);
uint32_t pcg32Next(PCG32* rng);
void pcg32Seed(PCG32* rng, uint64_t seed, uint64_t sequence);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PCG32_H
