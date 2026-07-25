#include "attacks.h"

// Std includes
#include <stdio.h>
#include <stdlib.h>

// Constants
const uint64_t bishopMagics[64] = {0x821518401104018ull, 0x100328c320b06014ull, 0x18c1914180602480ull, 0x22aa0160100100ull, 0x1104000003020ull, 0xa000a253a8021008ull, 0xa050514a0a130000ull, 0x6c0d1210820803c4ull, 0x20000a0265050138ull, 0x42203065460b060ull, 0x4000418300c05040ull, 0x110022aa016122a0ull, 0x10080c1044059000ull, 0x507000a254270084ull, 0x80200512509a811ull, 0x680030928a0984ull, 0xc238000606862140ull, 0x401c000d018140c4ull, 0x8003400240e09ull, 0x584802008a044040ull, 0x4001000820080801ull, 0x200e04071424000ull, 0x803809863030508ull, 0x1c10831818280ull, 0x281040005a050308ull, 0x80a108229450220ull, 0x2a91010010004604ull, 0x48080010620020ull, 0x1004204004040ull, 0x132048018080500ull, 0x1112851082222a10ull, 0x20480102c5870ull, 0x2304fb00202000ull, 0x1430d28501040ull, 0x400680800040020ull, 0x4001400a00002200ull, 0x6008008400010500ull, 0x8060080080004040ull, 0x28181a0040400ull, 0x601140c5d1010400ull, 0x801828c0641b801ull, 0x401818283081c00ull, 0x20001828d8001000ull, 0x60224026001020ull, 0x180098500b00400ull, 0x3820200d001a0ull, 0x100c02e180611380ull, 0x2430149803041c0ull, 0x689426a05050300ull, 0x81413282432004ull, 0x10c0c1418000ull, 0x80800c0a0a060000ull, 0x410109408e39100ull, 0x204027142240a010ull, 0x213144511405280ull, 0x1089850500942814ull, 0x800018820111a100ull, 0x190014142624300ull, 0x104820c0c14188ull, 0x1010400a2420200ull, 0x10009809408e390ull, 0x40058007844621c0ull, 0x4800130a2a112060ull, 0xd1404c286003024ull};
const uint64_t rookMagics[64] = {0x80004000108020ull, 0x4008c410002000ull, 0x82000c884040820ull, 0x4420080402020421ull, 0x1020192202010020ull, 0x2001c0810020001ull, 0xa80010022000880ull, 0x2800041000a2080ull, 0x800320400080ull, 0x122402010102040ull, 0x2820016004c8056ull, 0x406000a0046204cull, 0x2001212020408ull, 0x2001d05060010ull, 0x2204c80810204ull, 0x860010a0646482ull, 0x2404622008100820ull, 0x80a2010102040ull, 0x9601030010442004ull, 0x4000090010002102ull, 0x628008004004880ull, 0x101010004000802ull, 0x12000400014948d0ull, 0x1408600004500a4ull, 0x238480004000ull, 0x40002000c002d000ull, 0x11308200420020ull, 0x804ad00100082300ull, 0x110100080004ull, 0x2402000200048831ull, 0x210222400411008ull, 0x8400c8200064104ull, 0x4040028020800c40ull, 0x800208501004001ull, 0x4406002082001542ull, 0x402202002830ull, 0x880080800400ull, 0x400800400802a00ull, 0x802011004000288ull, 0xc8402001151ull, 0x1800800140008020ull, 0x2020101034c000ull, 0x20081c0120020ull, 0x21000a1010008ull, 0x40202040a0a0020ull, 0x1201040002008080ull, 0x1808218a040010ull, 0x8010008044020011ull, 0x400420030b09200ull, 0x400030b09300ull, 0x20002a00306b1600ull, 0x20002600290c9200ull, 0x802020408121200ull, 0x280040003050300ull, 0x50c8c941500400ull, 0x2008404030801840ull, 0x1990052004c2416ull, 0xd010031802a026aull, 0x8002002680280a12ull, 0xc400a001806ull, 0x40c8000300040b29ull, 0x220a004c18151002ull, 0x640018810016344ull, 0x52091080c1040226ull};

// Precomputed relevant occupancy masks
uint64_t bishopMasks[64];
uint64_t rookMasks[64];

// Precomputed relevant bits for each square
uint32_t bishopRelevantBits[64];
uint32_t rookRelevantBits[64];

// Magic bitboards' attack tables
uint64_t bishopAttacks[5009], * bishopTablePointers[64];
uint64_t rookAttacks[102359], * rookTablePointers[64];

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
