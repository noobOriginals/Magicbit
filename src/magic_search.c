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

uint64_t findBishopMagic(uint32_t square, PCG32* rng, uint32_t* minBits, uint32_t* arrSize) {
    PCG32* newRng = createPCG32();
    newRng->state = rng->state;
    newRng->inc = rng->inc;
    uint64_t table[512] = {};
    uint32_t bits = bishopRelevantBits[square];
    uint32_t size = (1u << bits);
    uint64_t lastGoodMagic = 0;
    int32_t failed = 0;
    *arrSize = 10000000;
    while (!failed) {
        failed = 1;
        for (uint32_t try = 0; try < 10; try += 1) {
            uint64_t magic = randomMagic(newRng);
            if (popcount64((magic * (bocc[square * 512 + size - 1] | 1)) & 0xFF00000000000000ull) < 6) continue;
            uint32_t maxIdx = 0;
            int32_t good = 1;
            for (uint32_t i = 0; i < size; i += 1) table[i] = 0;
            for (uint32_t i = 0; i < size; i += 1) {
                uint32_t index = (uint32_t) ((magic * bocc[square * 512 + i]) >> (64 - bits));
                if (index > maxIdx) maxIdx = index;
                if (table[index] == batt[square * 512 + i]) continue;
                if (table[index]) {
                    good = 0;
                    break;
                }
                table[index] = batt[square * 512 + i];
            }
            if (good && *arrSize > maxIdx + 1) {
                lastGoodMagic = magic;
                *minBits = bits;
                *arrSize = maxIdx + 1;
                failed = 0;
                break;
            }
        }
        if (failed && size == (1u << bits)) {
            failed = 0;
        } else {
            bits -= 1;
        }
    }
    destroyPCG32(newRng);
    return lastGoodMagic;
}

uint64_t findRookMagic(uint32_t square, PCG32* rng) {
    uint64_t table[4096] = {};
    uint32_t bits = rookRelevantBits[square];
    uint32_t size = (1u << bits);
    for (;;) {
        uint64_t magic = randomMagic(rng);
        if (popcount64((magic * (rocc[square * 4096 + size - 1] | 1)) & 0xFF00000000000000ull) < 6) continue;
        int32_t good = 1;
        for (uint32_t i = 0; i < size; i += 1) table[i] = 0;
        for (uint32_t i = 0; i < size; i += 1) {
            uint32_t index = (uint32_t) ((magic * rocc[square * 4096 + i]) >> (64 - bits));
            if (table[index] == ratt[square * 4096 + i]) continue;
            if (table[index]) {
                good = 0;
                break;
            }
            table[index] = ratt[square * 4096 + i];
        }
        if (good) return magic;
    }
}

void bishopMagicSearch(uint64_t magics[64], uint32_t bits[64], PCG32* rng, uint32_t* totalSize) {
    *totalSize = 0;
    uint32_t size;
    for (uint32_t i = 0; i < 64; i += 1) {
        magics[i] = findBishopMagic(i, rng, bits + i, &size);
        *totalSize += size;
        randomMagic(rng);
    }
}

void rookMagicSearch() {
    PCG32* rng = createPCG32();
    pcg32Seed(rng, pcgHash(7838234), pcgHash(91247124));

    printf("Rook magic search:\n{");
    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < 64; i++) {
        printf("0x%llxull, ", findRookMagic(i, rng));
        totalSize += (uint32_t) (1ull << rookRelevantBits[i]);
    }
    printf("};\nTotal size: %u\n\n", totalSize);

    destroyPCG32(rng);
}
