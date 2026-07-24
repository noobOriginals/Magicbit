#ifndef MAGIC_SEARCH_H
#define MAGIC_SEARCH_H

// Std includes
#include <stdint.h>

// Local includes
#include "pcg32.h"

#ifdef __cplusplus
extern "C" {
#endif

void initAttackSubsets();
uint64_t findBishopMagic(uint32_t square, PCG32* rng, uint32_t* minBits, uint32_t* arrSize);
uint64_t findRookMagic(uint32_t square, PCG32* rng);
void bishopMagicSearch(uint64_t magics[64], uint32_t bits[64], PCG32* rng, uint32_t* totalSize);
void rookMagicSearch();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MAGIC_SEARCH_H
