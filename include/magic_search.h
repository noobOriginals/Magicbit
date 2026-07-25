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
uint64_t findBishopMagic(uint32_t square, PCG32* rng, uint32_t* arrSize, uint32_t* unusedBits);
uint64_t findRookMagic(uint32_t square, PCG32* rng, uint32_t* arrSize, uint32_t* unusedBits);
uint64_t findBetterMagic(uint32_t square, PCG32* rng, uint32_t maxSize, int32_t rook, uint32_t* size, uint32_t* unusedBits, int32_t* terminate);
void bishopMagicSearch(uint64_t magics[64], PCG32* rng, uint32_t* totalSize);
void rookMagicSearch(uint64_t magics[64], PCG32* rng, uint32_t* totalSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MAGIC_SEARCH_H
