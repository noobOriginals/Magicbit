#ifndef ATTACKS_H
#define ATTACKS_H

// Std includes
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Utility
#define FILE_A 0b0000000100000001000000010000000100000001000000010000000100000001ull
#define FILE_H 0b1000000010000000100000001000000010000000100000001000000010000000ull
#define RANK_1 0b0000000000000000000000000000000000000000000000000000000011111111ull
#define RANK_8 0b1111111100000000000000000000000000000000000000000000000000000000ull
#if defined(_MSC_VER)
    #include <intrin.h>
    #define popcount64(x) __popcnt64(x)
#else
    #define popcount64(x) __builtin_popcountll(x)
#endif

// Constants
extern const uint64_t bishopMagics[64];
extern const uint64_t rookMagics[64];

// Precomputed relevant occupancy masks
extern uint64_t bishopMasks[64];
extern uint64_t rookMasks[64];

// Precomputed relevant bits for each square
extern uint32_t bishopRelevantBits[64];
extern uint32_t rookRelevantBits[64];

// Magic bitboards' attack tables
extern uint64_t bishopAttacks[5248], * bishopTablePointers[64];
extern uint64_t rookAttacks[102400], * rookTablePointers[64];

// Initialize attack tables
uint64_t getBishopAttacksSlow(uint32_t square, uint64_t occupancy); // Slow version
uint64_t getRookAttacksSlow(uint32_t square, uint64_t occupancy); // Slow version
void initAttackTables();

// Access attack tables
uint64_t getBishopAttacks(uint32_t square, uint64_t occupancies);
uint64_t getRookAttacks(uint32_t square, uint64_t occupancies);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ATTACKS_H
