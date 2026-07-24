#include <iostream>
#include <chrono>

#include "attacks.h"
#include "magic_search.h"

// Utility
static const char* boardVisualTemplate =
"     a   b   c   d   e   f   g   h     \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 8 | x | y |   |   |   |   |   | z | 8 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 7 | w |   |   |   |   |   |   |   | 7 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 6 |   |   |   |   |   |   |   |   | 6 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 5 |   |   |   |   |   |   |   |   | 5 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 4 |   |   |   |   |   |   |   |   | 4 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 3 |   |   |   |   |   |   |   |   | 3 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 2 |   |   |   |   |   |   |   |   | 2 \n"
"   +---+---+---+---+---+---+---+---+   \n"
" 1 |   |   |   |   |   |   |   |   | 1 \n"
"   +---+---+---+---+---+---+---+---+   \n"
"     a   b   c   d   e   f   g   h     \n"; // Visual template that gets copied to the given buffer

// Returns the visual string representation of the given bitboard, ready to be printed to the console, or file or anything else
int32_t getVisualBitboardString(uint64_t bitboard, char* str, uint64_t size) {
    // Copy the visual template
    if (size < strlen(boardVisualTemplate)) return 1;
    memcpy(str, boardVisualTemplate, strlen(boardVisualTemplate));

    uint32_t vidx = 85; // First index of a square in the visual template
    for (uint32_t rank = 8; rank > 0; rank--) {
        for (uint32_t file = 0; file < 8; file += 1) {
            uint32_t square = (rank - 1) * 8 + file;
            char piece = ' ';
            if (bitboard & (1ull << square)) {
                piece = '1';
            }
            str[vidx] = piece;
            vidx += 4; // File offset in template visual
        }
        vidx += 48; // Rank offset from last file to first file in template visual
    }
    return 0;
}

int32_t assertPositions(uint64_t actual, uint64_t expected) {
    if (actual != expected) {
        char expectedBuffer[1024] = {}, actualBuffer[1024] = {};
        getVisualBitboardString(expected, expectedBuffer, 1024);
        getVisualBitboardString(actual, actualBuffer, 1024);
        printf("Position assert failed!\n.Expected: %s\nActual: %s\n", expectedBuffer, actualBuffer);
        return 0;
    }
    return 1;
}

void testMagicBitboards() {
    for (uint32_t sq = 0; sq < 64; sq++) {
        printf("Testing bishop attacks, square %u\n", sq);
        uint64_t mask = bishopMasks[sq];
        uint64_t subset = 0;
        do {
            if (!assertPositions(getBishopAttacks(sq, subset), getBishopAttacksSlow(sq, subset))) exit(1);
            subset = (subset - mask) & mask;
        } while (subset != 0);
    }
    for (uint32_t sq = 0; sq < 64; sq++) {
        printf("Testing rook attacks, square %u\n", sq);
        uint64_t mask = rookMasks[sq];
        uint64_t subset = 0;
        do {
            if (!assertPositions(getRookAttacks(sq, subset), getRookAttacksSlow(sq, subset))) exit(1);
            subset = (subset - mask) & mask;
        } while (subset != 0);
    }
}

void testMagicBitboardsVsSlowVersion() {
    uint32_t iterations = 100000;

    std::cout << "Timing magic bitboards, " << iterations << " iterations\n";
    auto startTime = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < iterations; i++) {
        for (uint32_t sq = 0; sq < 64; sq++) {
            uint64_t mask = bishopMasks[sq];
            uint64_t subset = 0;
            do {
                getBishopAttacks(sq, subset);
                subset = (subset - mask) & mask;
            } while (subset != 0);
        }
        for (uint32_t sq = 0; sq < 64; sq++) {
            uint64_t mask = rookMasks[sq];
            uint64_t subset = 0;
            do {
                getRookAttacks(sq, subset);
                subset = (subset - mask) & mask;
            } while (subset != 0);
        }
    }
    std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - startTime;
    std::cout << "Took " << elapsed.count() / 1000.0f << " seconds\n";

    std::cout << "Timing slow bitboards, " << iterations << " iterations\n";
    startTime = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < iterations; i++) {
        for (uint32_t sq = 0; sq < 64; sq++) {
            uint64_t mask = bishopMasks[sq];
            uint64_t subset = 0;
            do {
                getBishopAttacksSlow(sq, subset);
                subset = (subset - mask) & mask;
            } while (subset != 0);
        }
        for (uint32_t sq = 0; sq < 64; sq++) {
            uint64_t mask = rookMasks[sq];
            uint64_t subset = 0;
            do {
                getRookAttacksSlow(sq, subset);
                subset = (subset - mask) & mask;
            } while (subset != 0);
        }
    }
    elapsed = std::chrono::high_resolution_clock::now() - startTime;
    std::cout << "Took " << elapsed.count() / 1000.0f << " seconds\n";
}

void printMagics(const uint64_t magics[64], const uint32_t bits[64], const uint32_t totalSize, const bool rook) {
    if (rook) {
        printf("Rook magic search:\n{");
    } else {
        printf("Bishop magic search:\n{");
    }
    for (uint32_t i = 0; i < 63; i++) {
        printf("0x%llxull, ", magics[i]);
    }
    printf("0x%llxull", magics[63]);
    printf("};\n{");
    uint32_t totalBits = 0;
    for (uint32_t i = 0; i < 63; i++) {
        printf("%u, ", bits[i]);
        totalBits += bits[i];
    }
    printf("%u", bits[63]);
    totalBits += bits[63];
    printf("};\nTotal size: %u\nTotal bits: %u\n\n", totalSize, totalBits);
}

int main() {
    initAttackTables();
    initAttackSubsets();

    printf("Original magics:\n");
    printMagics(bishopMagics, bishopRelevantBits, sizeof(bishopAttacks) / sizeof(uint64_t), false);

    uint64_t magics[64] = {};
    uint32_t bits[64] = {};
    PCG32* rng = createPCG32();
    pcg32Seed(rng, pcgHash(442341), pcgHash(976233));
    uint32_t totalSize, bestSize;

    auto startTime = std::chrono::high_resolution_clock::now();
    bishopMagicSearch(magics, bits, rng, &bestSize);
    printMagics(magics, bits, bestSize, false);
    for (uint32_t i = 0; i < 10000; i++) {
        bishopMagicSearch(magics, bits, rng, &totalSize);
        if (totalSize < bestSize) {
            bestSize = totalSize;
            printMagics(magics, bits, bestSize, false);
        }
    }
    std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - startTime;
    std::cout << "Took " << elapsed.count() / 1000.0f << " seconds\n";
    return 0;
}
