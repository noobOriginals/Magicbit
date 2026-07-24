#include "attacks.h"

// Std includes
#include <stdio.h>
#include <stdlib.h>

// Constants
const uint64_t bishopMagics[64] = {0x10a00100420042ull, 0x10041090820890ull, 0x2101010e006002ull, 0x44400846004d0ull, 0x204042000009680ull, 0x40821140000000ull, 0x5211010049000ull, 0x111040444020800ull, 0x208020200c230048ull, 0x81210800a10040ull, 0x40400860140ull, 0x1000080e81000090ull, 0x428540420001480ull, 0x20211040004ull, 0xc000015208201924ull, 0x480ca0202020202ull, 0x200088204c1ac0ull, 0x8201001081882ull, 0x2010082a440040c0ull, 0x7008000086004010ull, 0x2024001094200088ull, 0x600280011000a000ull, 0x8033010148029020ull, 0x2429040104a4ull, 0x8024844060081000ull, 0x2088020080083ull, 0x800900022016601ull, 0x8100c4004040008ull, 0x1005007004000ull, 0x2008042482000ull, 0x4002060000849010ull, 0x4410908003040382ull, 0xc010080440081101ull, 0x80841040047000ull, 0x164800040804ull, 0x8001008080080204ull, 0x2408430040040040ull, 0x4080020920080ull, 0x3010104002a0210ull, 0x2004850440010c00ull, 0x14040242000844ull, 0x80040504302012c0ull, 0x1008024881000ull, 0x2000004010408a00ull, 0x400812001040ull, 0xc0010041000081ull, 0x12b00602804200ull, 0x4b08081008200ull, 0x82012120b06012ull, 0x2840082110200ull, 0x112020201040069ull, 0xcb1014841c0000ull, 0xa2481102020000ull, 0x4c090010a4800ull, 0x40100220991640ull, 0x12801280a005000ull, 0x3000220114202200ull, 0x2401041060ull, 0x2000000084480880ull, 0x2448000009048800ull, 0x4004200a0204100ull, 0x418990101082ull, 0x8044808082086ull, 0x40040810410020ull};
const uint64_t rookMagics[64] = {0x180006080400010ull, 0xc0004020007000ull, 0x18010008020000aull, 0xd00082004d00100ull, 0x1180028008000400ull, 0x70022080c000100ull, 0x300319200040100ull, 0x818003000020c980ull, 0x404800280694004ull, 0x808040002000ull, 0x4280100080e000ull, 0x1000900201000ull, 0x8111800400810800ull, 0x4002801200040080ull, 0x14000854120110ull, 0x1000080610002ull, 0x40018020c08000ull, 0x2020008020400080ull, 0x2002020010802042ull, 0x600848010000801ull, 0x100b110004080100ull, 0x1010008020400ull, 0x20004400c1021008ull, 0x2520000408c01ull, 0x12401080088121ull, 0x2040002060081000ull, 0x1009004900102000ull, 0x4010001080080080ull, 0x40080080080ull, 0x2002000200100508ull, 0x1000110400021098ull, 0x10080018000e100ull, 0x7081400920800488ull, 0x400081802000ull, 0x1000844012002200ull, 0xc010100080800800ull, 0x403001005002801ull, 0x8000040080800200ull, 0x2004102008408ull, 0x8102000044ull, 0x3000800040008020ull, 0x1001104200820020ull, 0x4204814200160020ull, 0x4000080010008080ull, 0x6001020060008ull, 0x1001204084080110ull, 0x8000802110400b0ull, 0x90c90408406000bull, 0x8801240042080ull, 0x167004180220e00ull, 0x5002000401100ull, 0xcd00480080300080ull, 0x40800110100ull, 0x5808804400020080ull, 0x1000200040100ull, 0x4002800100104080ull, 0x404104421020082ull, 0x8000400100208011ull, 0x100300402810a003ull, 0x2000200810000501ull, 0xa000820041002ull, 0x2000410010802ull, 0x200d0210804ull, 0x2001240100408022ull};

// Precomputed relevant occupancy masks
uint64_t bishopMasks[64];
uint64_t rookMasks[64];

// Precomputed relevant bits for each square
uint32_t bishopRelevantBits[64];
uint32_t rookRelevantBits[64];

// Magic bitboards' attack tables
uint64_t bishopAttacks[5248], * bishopTablePointers[64];
uint64_t rookAttacks[102400], * rookTablePointers[64];

// Utility
static uint64_t getSlidingAttacks(uint32_t square, uint64_t occupancy, const int32_t* rankDirs, const int32_t* fileDirs, const uint32_t numDirs) {
    uint64_t attacks = 0;
    for (uint32_t d = 0; d < numDirs; d += 1) {
        int32_t rank = square / 8, file = square % 8;
        for (;;) {
            rank += rankDirs[d];
            file += fileDirs[d];
            if (rank > 7 || rank < 0 || file > 7 || file < 0) break;
            uint32_t reached = (uint32_t) (rank * 8 + file);
            attacks |= (1ull << reached);
            if (occupancy & (1ull << reached)) break;
        }
    }
    return attacks;
}

uint64_t getBishopAttacksSlow(uint32_t square, uint64_t occupancy) {
    if (square > 63) return 0;
    int32_t rankDirs[4] = { 1, -1,  1, -1};
    int32_t fileDirs[4] = { 1, -1, -1,  1};
    return getSlidingAttacks(square, occupancy, rankDirs, fileDirs, 4);
}

uint64_t getRookAttacksSlow(uint32_t square, uint64_t occupancy) {
    if (square > 63) return 0;
    int32_t rankDirs[4] = { 1, -1,  0,  0};
    int32_t fileDirs[4] = { 0,  0,  1, -1};
    return getSlidingAttacks(square, occupancy, rankDirs, fileDirs, 4);
}

static void initOccupancyMasks() {
    // Rook individual ray dirs
    int32_t vertRankDirs[2] = { 1, -1};
    int32_t vertFileDirs[2] = { 0,  0};
    int32_t horzRankDirs[2] = { 0,  0};
    int32_t horzFileDirs[2] = { 1, -1};

    // Compute mask for each square
    for (uint32_t sq = 0; sq < 64; sq += 1) {
        bishopMasks[sq] = getBishopAttacksSlow(sq, 0) & ~(FILE_A | FILE_H | RANK_1 | RANK_8);
        rookMasks[sq] = (getSlidingAttacks(sq, 0, vertRankDirs, vertFileDirs, 2) & ~(RANK_1 | RANK_8)) |
                        (getSlidingAttacks(sq, 0, horzRankDirs, horzFileDirs, 2) & ~(FILE_A | FILE_H));

        // Count number of relevant bits
        bishopRelevantBits[sq] = (uint32_t) popcount64(bishopMasks[sq]);
        rookRelevantBits[sq] = (uint32_t) popcount64(rookMasks[sq]);
    }
}

// Initialize attack tables
void initAttackTables() {
    initOccupancyMasks();
    {
        // Bishop attack table
        uint32_t tableIndex = 0;
        for (uint32_t square = 0; square < 64; square += 1) {
            bishopTablePointers[square] = &bishopAttacks[tableIndex];
            uint64_t mask = bishopMasks[square], occ[512] = {}, att[512] = {};
            uint64_t subset = 0;
            occ[0] = subset;
            att[0] = getBishopAttacksSlow(square, subset);
            uint32_t size = 1, bits = bishopRelevantBits[square];
            while (subset != mask) {
                subset = (subset - mask) & mask;
                occ[size] = subset;
                att[size] = getBishopAttacksSlow(square, subset);
                size += 1;
            }
            if (size != (1ull << bits)) {
                fprintf(stderr, "FATAL: bishop table build mismatch at square %u\n", square);
                exit(1);
            }

            uint32_t maxIdx = 0;
            for (uint32_t i = 0; i < size; i += 1) {
                uint32_t index = (uint32_t) ((bishopMagics[square] * occ[i]) >> (64 - bits));
                if (index + 1 > maxIdx) maxIdx = index + 1;
                bishopTablePointers[square][index] = att[i];
            }
            tableIndex += maxIdx;
        }
    }
    {
        // Rook attack table
        uint32_t tableIndex = 0;
        for (uint32_t square = 0; square < 64; square += 1) {
            rookTablePointers[square] = &rookAttacks[tableIndex];
            uint64_t mask = rookMasks[square], occ[4096] = {}, att[4096] = {};
            uint64_t subset = 0;
            occ[0] = subset;
            att[0] = getRookAttacksSlow(square, subset);
            uint32_t size = 1, bits = rookRelevantBits[square];
            while (subset != mask) {
                subset = (subset - mask) & mask;
                occ[size] = subset;
                att[size] = getRookAttacksSlow(square, subset);
                size += 1;
            }
            if (size != (1ull << bits)) {
                fprintf(stderr, "FATAL: rook table build mismatch at square %u\n", square);
                exit(1);
            }

            uint32_t maxIdx = 0;
            for (uint32_t i = 0; i < size; i += 1) {
                uint32_t index = (uint32_t) ((rookMagics[square] * occ[i]) >> (64 - bits));
                if (index + 1 > maxIdx) maxIdx = index + 1;
                rookTablePointers[square][index] = att[i];
            }
            tableIndex += maxIdx;
        }
    }
}

// Access attack tables
uint64_t getBishopAttacks(uint32_t square, uint64_t occupancies) {
    if (square > 63) return 0;
    occupancies &= bishopMasks[square];
    occupancies *= bishopMagics[square];
    occupancies >>= (64 - bishopRelevantBits[square]);
    return bishopTablePointers[square][occupancies];
}

uint64_t getRookAttacks(uint32_t square, uint64_t occupancies) {
    if (square > 63) return 0;
    occupancies &= rookMasks[square];
    occupancies *= rookMagics[square];
    occupancies >>= (64 - rookRelevantBits[square]);
    return rookTablePointers[square][occupancies];
}
