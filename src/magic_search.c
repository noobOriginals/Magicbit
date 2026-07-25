#include "magic_search.h"

// Std includes
#include <stdlib.h>
#include <stdio.h>

// Local includes
#include "attacks.h"

// Precomputed attack subsets
static uint64_t bocc[32768], batt[32768];
static uint64_t rocc[262144], ratt[262144];

static uint64_t randomU64(PCG32* rng) {
    return (uint64_t) pcg32Next(rng) << 32 | (uint64_t) pcg32Next(rng);
}

static uint64_t randomMagic(PCG32* rng) {
    return randomU64(rng) & randomU64(rng) & randomU64(rng);
}

void initAttackSubsets() {
    {
        // Bishop
        for (uint32_t square = 0; square < 64; square += 1) {
            uint64_t mask = bishopMasks[square];
            uint64_t subset = 0;
            bocc[square * 512] = subset;
            batt[square * 512] = getBishopAttacksSlow(square, subset);
            uint32_t size = 1, bits = bishopRelevantBits[square];
            while (subset != mask) {
                subset = (subset - mask) & mask;
                bocc[square * 512 + size] = subset;
                batt[square * 512 + size] = getBishopAttacksSlow(square, subset);
                size += 1;
            }
            if (size != (1u << bits)) {
                printf("Failed to generate correct number of bishop attacks sets\n");
                exit(1);
            }
        }
    }
    {
        // Rook
        for (uint32_t square = 0; square < 64; square += 1) {
            uint64_t mask = rookMasks[square];
            uint64_t subset = 0;
            rocc[square * 4096] = subset;
            ratt[square * 4096] = getRookAttacksSlow(square, subset);
            uint32_t size = 1, bits = rookRelevantBits[square];
            while (subset != mask) {
                subset = (subset - mask) & mask;
                rocc[square * 4096 + size] = subset;
                ratt[square * 4096 + size] = getRookAttacksSlow(square, subset);
                size += 1;
            }
            if (size != (1u << bits)) {
                printf("Failed to generate correct number of rook attacks sets\n");
                exit(1);
            }
        }
    }
}

uint64_t findBishopMagic(uint32_t square, PCG32* rng, uint32_t* arrSize, uint32_t* unusedBits) {
    uint64_t table[512] = {};
    uint32_t bits = bishopRelevantBits[square];
    uint32_t size = (1u << bits);
    uint32_t sparseness = (bits >= 8) ? 2 : 3;
    for (;;) {
        uint64_t magic = randomMagic(rng);
        // magic = 0xfc0962854a77f576ull;
        uint32_t maxIdx = 0;
        uint32_t unused = 0b111111111 >> (9 - bits);
        int32_t good = 1;
        for (uint32_t i = 0; i < size; i += 1) table[i] = 0;
        for (uint32_t i = 0; i < size; i += 1) {
            uint32_t index = (uint32_t) ((magic * bocc[square * 512 + i]) >> (64 - bits));
            unused &= ~index;
            if (index > maxIdx) maxIdx = index;
            if (table[index] == batt[square * 512 + i]) continue;
            if (table[index]) {
                good = 0;
                break;
            }
            table[index] = batt[square * 512 + i];
        }
        if (good) {
            *arrSize = maxIdx + 1;
            *unusedBits = popcount64(unused);
            return magic;
        }
    }
}

uint64_t findRookMagic(uint32_t square, PCG32* rng, uint32_t* arrSize, uint32_t* unusedBits) {
    uint64_t table[4096] = {};
    uint32_t bits = rookRelevantBits[square];
    uint32_t size = (1u << bits);
    uint32_t sparseness = (bits >= 12) ? 2 : 3;
    for (;;) {
        uint64_t magic = randomMagic(rng);
        uint32_t maxIdx = 0;
        uint32_t unused = 0b111111111 >> (12 - bits);
        int32_t good = 1;
        for (uint32_t i = 0; i < size; i += 1) table[i] = 0;
        for (uint32_t i = 0; i < size; i += 1) {
            uint32_t index = (uint32_t) ((magic * rocc[square * 4096 + i]) >> (64 - bits));
            unused &= ~index;
            if (index > maxIdx) maxIdx = index;
            if (table[index] == ratt[square * 4096 + i]) continue;
            if (table[index]) {
                good = 0;
                break;
            }
            table[index] = ratt[square * 4096 + i];
        }
        if (good) {
            *arrSize = maxIdx + 1;
            *unusedBits = popcount64(unused);
            return magic;
        }
    }
}

uint64_t findBetterMagic(uint32_t square, PCG32* rng, uint32_t maxSize, int32_t rook, uint32_t* size, uint32_t* unusedBits, int32_t* terminate) {
    uint64_t table[4096] = {};
    uint64_t* occ = rook ? &rocc[square * 4096] : &bocc[square * 512];
    uint64_t* att = rook ? &ratt[square * 4096] : &batt[square * 512];
    uint32_t bits = rook ? rookRelevantBits[square] : bishopRelevantBits[square];
    uint32_t occSize = (1u << bits);
    for (;;) {
        uint64_t magic = randomMagic(rng);
        uint32_t maxIdx = 0;
        uint32_t unused = 0b111111111111 >> (12 - bits);
        int32_t good = 1;
        for (uint32_t i = 0; i < occSize; i += 1) table[i] = 0;
        for (uint32_t i = 0; i < occSize; i += 1) {
            uint32_t idx = (uint32_t) ((magic * occ[i]) >> (64 - bits));
            if (idx + 1 >= maxSize && !(*terminate)) {
                good = 0;
                break;
            }
            unused &= ~idx;
            if (idx > maxIdx) maxIdx = idx;
            if (table[idx] == att[i]) continue;
            if (table[idx] != 0) {
                good = 0;
                break;
            }
            table[idx] = att[i];
        }
        if (good) {
            *size = maxIdx + 1;
            *unusedBits = popcount64(unused);
            return magic;
        }
    }
}

void bishopMagicSearch(uint64_t magics[64], PCG32* rng, uint32_t* totalSize) {
    *totalSize = 0;
    uint32_t size;
    for (uint32_t i = 0; i < 64; i += 1) {
        magics[i] = findBishopMagic(i, rng, &size, NULL);
        *totalSize += size;
        randomMagic(rng);
    }
}

void rookMagicSearch(uint64_t magics[64], PCG32* rng, uint32_t* totalSize) {
    *totalSize = 0;
    uint32_t size;
    for (uint32_t i = 0; i < 64; i += 1) {
        magics[i] = findRookMagic(i, rng, &size, NULL);
        *totalSize += size;
        randomMagic(rng);
    }
}
