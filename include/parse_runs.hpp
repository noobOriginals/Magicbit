#ifndef PARSE_RUNS_HPP
#define PARSE_RUNS_HPP

#include <cstdint>
#include <vector>

#include "attacks.h"

void concat_magics(std::vector<std::vector<uint64_t>> arrays, uint64_t magics[64], uint32_t& totalSize, bool rook) {
    std::vector<uint32_t> sizes(64, 5000);
    for (uint32_t arr = 0; arr < arrays.size(); arr++) {
        for (uint32_t sq = 0; sq < 64; sq++) {
            uint64_t mask = rook ? rookMasks[sq] : bishopMasks[sq];
            uint64_t occ[4096] = {}, att[4096] = {};
            uint64_t subset = 0;
            uint32_t size = 0;
            do {
                occ[size] = subset;
                att[size] = rook ? getRookAttacks(sq, subset) : getBishopAttacks(sq, subset);
                size++;
                subset = (subset - mask) & mask;
            } while (subset != 0);

            uint64_t magic = arrays[arr][sq];
            uint32_t bits = rook ? rookRelevantBits[sq] : bishopRelevantBits[sq], maxIdx = 0;
            for (uint32_t i = 0; i < size; i++) {
                uint32_t idx = (uint32_t) ((magic * occ[i]) >> (64 - bits));
                if (idx > maxIdx) maxIdx = idx;
            }
            maxIdx++;

            if (maxIdx < sizes[sq]) {
                sizes[sq] = maxIdx;
                magics[sq] = magic;
            }
        }
    }
    totalSize = 0;
    for (auto& x : sizes) totalSize += x;
}

#endif // PARSE_RUNS_HPP
