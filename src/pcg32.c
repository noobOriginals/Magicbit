#include "pcg32.h"

// Std includes
#include <stdlib.h>

PCG32* createPCG32() {
    return (PCG32*) calloc(1, sizeof(PCG32));
}

void destroyPCG32(PCG32* rng) {
    free(rng);
}

uint64_t pcgHash(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

uint32_t pcg32Next(PCG32* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ull + (rng->inc | 1ull);
    uint32_t xorshift = (uint32_t) (((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rotation = (uint32_t) (oldstate >> 59u);
    return (xorshift >> rotation) | (xorshift << ((32u - rotation) & 31u));
}

void pcg32Seed(PCG32* rng, uint64_t seed, uint64_t sequence) {
    rng->state = 0ull;
    rng->inc = (sequence << 1u) | 1u;
    pcg32Next(rng);
    rng->state += seed;
    pcg32Next(rng);
}
