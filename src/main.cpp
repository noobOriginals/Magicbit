#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <string>

#include "attacks.h"
#include "magic_search.h"
#include "parse_runs.hpp"

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
    if (bits) {
        printf("};\n{");
        uint32_t totalBits = 0;
        for (uint32_t i = 0; i < 63; i++) {
            printf("%u, ", bits[i]);
            totalBits += bits[i];
        }
        printf("%u", bits[63]);
        totalBits += bits[63];
        printf("};\nTotal size: %u\nTotal bits: %u\n\n", totalSize, totalBits);
    } else {
        printf("};\nTotal size: %u\n\n", totalSize);
    }
}

void fprintMagics(FILE* file, const uint64_t magics[64], const uint32_t bits[64], const uint32_t totalSize, const bool rook) {
    if (rook) {
        fprintf(file, "Rook magic search:\n{");
    } else {
        fprintf(file, "Bishop magic search:\n{");
    }
    for (uint32_t i = 0; i < 63; i++) {
        fprintf(file, "0x%llxull, ", magics[i]);
    }
    fprintf(file, "0x%llxull", magics[63]);
    if (bits) {
        fprintf(file, "};\n{");
        uint32_t totalBits = 0;
        for (uint32_t i = 0; i < 63; i++) {
            fprintf(file, "%u, ", bits[i]);
            totalBits += bits[i];
        }
        fprintf(file, "%u", bits[63]);
        totalBits += bits[63];
        fprintf(file, "};\nTotal size: %u\nTotal bits: %u\n\n", totalSize, totalBits);
    } else {
        fprintf(file, "};\nTotal size: %u\n\n", totalSize);
    }
}

uint64_t tryBishopSquare(uint32_t square, uint32_t* size, uint32_t* bits) {
    PCG32* rng = createPCG32();
    pcg32Seed(rng, pcgHash(time(NULL)), pcgHash(time(NULL) & 0x204042000009680ull));
    uint32_t totalSize, bestSize, unused;
    uint64_t magic, bestMagic;
    bestMagic = findBishopMagic(square, rng, &bestSize, &unused);
    for (uint32_t i = 0; i < 1000; i++) {
        magic = findBishopMagic(square, rng, &totalSize, &unused);
        if (totalSize < bestSize) {
            bestSize = totalSize;
            bestMagic = magic;
        }
    }
    destroyPCG32(rng);
    if (size) *size = bestSize;
    if (bits) *bits = unused;
    return bestMagic;
}

uint64_t tryRookSquare(uint32_t square, uint32_t* size, uint32_t* bits) {
    PCG32* rng = createPCG32();
    pcg32Seed(rng, pcgHash(0x445020000404cull), pcgHash(square));
    uint32_t totalSize, bestSize, unused;
    uint64_t magic, bestMagic;
    bestMagic = findRookMagic(square, rng, &bestSize, &unused);
    for (uint32_t i = 0; i < 1000; i++) {
        magic = findRookMagic(square, rng, &totalSize, &unused);
        if (totalSize < bestSize) {
            bestSize = totalSize;
            bestMagic = magic;
        }
    }
    destroyPCG32(rng);
    if (size) *size = bestSize;
    if (bits) *bits = unused;
    return bestMagic;
}

void findMagicsMultithread(bool rook) {
    uint64_t magics[64] = {};
    uint32_t size[64] = {};
    uint32_t bits[64] = {};

    std::queue<uint32_t> squares;
    for (uint32_t i = 0; i < 64; i++) squares.push(i);

    bool done = false;
    auto startTime = std::chrono::high_resolution_clock::now();
    auto monitor = [&]() {
        while (!done) {
            std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - startTime;
            std::printf("\r%u/64 done, elapsed time: %7.2fs", (uint32_t) (64 - squares.size()), elapsed.count() / 1000);
            std::fflush(stdout);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    std::thread monitorThread(monitor);

    std::mutex queueMutex;
    auto worker = [&]() {
        while (true) {
            uint32_t sq;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (squares.empty()) return;
                sq = squares.front();
                squares.pop();
            }
            magics[sq] = rook ? tryRookSquare(sq, &size[sq], &bits[sq]) : tryBishopSquare(sq, &size[sq], &bits[sq]);
        }
    };

    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++) threads.emplace_back(worker);
    for (auto& t : threads) t.join();
    done = true;
    monitorThread.join();
    printf("\n");

    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < 64; i++) totalSize += size[i];
    printMagics(magics, bits, totalSize, rook);
}

uint32_t searchBestSize(uint64_t* finalMagic, uint32_t square, bool rook, uint32_t timeLimitMillis, uint32_t runIdx) {
    std::atomic<uint32_t> bestSize(5000);
    std::atomic<uint64_t> bestMagic(0);
    std::atomic<uint32_t> workerIdx(0);
    std::atomic<bool> done(false);

    auto startTime = std::chrono::high_resolution_clock::now();
    auto monitor = [&]() {
        while (!done) {
            std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - startTime;
            if (elapsed.count() > timeLimitMillis) done.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    std::thread monitorThread(monitor);

    auto worker = [&]() {
        PCG32* rng = createPCG32();
        pcg32Seed(rng, pcgHash(0x445020000404cull + workerIdx++), pcgHash(square * runIdx));
        uint32_t totalSize, unused;
        uint64_t magic;
        while (true) {
            magic = rook ? findRookMagic(square, rng, &totalSize, &unused) : findBishopMagic(square, rng, &totalSize, &unused);
            if (totalSize < bestSize) {
                bestSize.store(totalSize);
                bestMagic.store(magic);
                printf("0x%llxull - Size: %u - Unused bits: %u\n", magic, bestSize.load(), unused);
            }
            if (unused >= 1 || done) {
                done.store(true);
                destroyPCG32(rng);
                return;
            }
        }
    };

    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++) threads.emplace_back(worker);
    for (auto& t : threads) t.join();
    monitorThread.join();
    *finalMagic = bestMagic.load();
    return bestSize.load();
}

void searchBestSizeTableMagics(bool rook, uint32_t timelimit, FILE* file, uint32_t runIdx) {
    uint64_t magics[64] = {};
    uint32_t totalSize = 0;
    for (uint32_t sq = 0; sq < 64; sq++) {
        printf("\nSquare %u\n", sq);
        totalSize += searchBestSize(&magics[sq], sq, rook, timelimit, runIdx);
    }
    printMagics(magics, NULL, totalSize, rook);
    fprintMagics(file, magics, NULL, totalSize, rook);
}

int main() {
    initAttackTables();
    initAttackSubsets();

    testMagicBitboards();

    // FILE* file = fopen("search_runs/concat.txt", "w");
    // {
    //     std::vector<std::vector<uint64_t>> arrays;
    //     arrays.push_back({0x821518401104018ull, 0x5014282809800ull, 0x18c1914180602480ull, 0x22aa0160100100ull, 0x1104000003020ull, 0xa000a253a8021008ull, 0xa050514a0a130000ull, 0x6c0d1210820803c4ull, 0x20000a0265050138ull, 0x2090322428a8098ull, 0x4000418300c05040ull, 0x110022aa016122a0ull, 0x10080c1044059000ull, 0x507000a254270084ull, 0x80200512509a811ull, 0x680030928a0984ull, 0xc238000606862140ull, 0x401c000d018140c4ull, 0x8003400240e09ull, 0x584802008a044040ull, 0x4001000820080801ull, 0x200e04071424000ull, 0x803809863030508ull, 0x1c10831818280ull, 0x281040005a050308ull, 0x80a108229450220ull, 0x2a91010010004604ull, 0x48080010620020ull, 0x1004204004040ull, 0x132048018080500ull, 0x1112851082222a10ull, 0x20480102c5870ull, 0x2304fb00202000ull, 0x1430d28501040ull, 0x400680800040020ull, 0x4001400a00002200ull, 0x6008008400010500ull, 0x8060080080004040ull, 0x28181a0040400ull, 0x601140c5d1010400ull, 0x801828c0641b801ull, 0x401818283081c00ull, 0x20001828d8001000ull, 0x60224026001020ull, 0x180098500b00400ull, 0x3820200d001a0ull, 0x100c02e180611380ull, 0x2430149803041c0ull, 0x689426a05050300ull, 0x81413282432004ull, 0x10c0c1418000ull, 0x80800c0a0a060000ull, 0x410109408e39100ull, 0x204027142240a010ull, 0x213144511405280ull, 0x1089850500942814ull, 0x800018820111a100ull, 0x190014142624300ull, 0x104820c0c14188ull, 0x1010400a2420200ull, 0x10009809408e390ull, 0x10000a853c220a2ull, 0x4800130a2a112060ull, 0x2080800c0823d08ull});
    //     arrays.push_back({0x821518401104018ull, 0x8205014261414008ull, 0x418300c4500820ull, 0x862aa0560118628ull, 0x111040b004408bull, 0x80a25427010020ull, 0x820090a509a80051ull, 0x10a3100c21000ull, 0x46084848505013aull, 0x108050155014096ull, 0x8260c18140c0c014ull, 0x180022aa05621008ull, 0x2081088a01022c1ull, 0x198000a253a80148ull, 0x180090aa099400ull, 0x8005052651420ull, 0x100c08060b060600ull, 0x401e0003660140c8ull, 0x808000400240e0cull, 0xa4000241020200ull, 0x3001000090400001ull, 0x1082000058050984ull, 0x210400106182cc3cull, 0x200009160c200ull, 0x422020001a050300ull, 0x8820170d2281a0ull, 0xc1000a2002440ull, 0x42080024004008ull, 0x421840200802000ull, 0x26020068491002ull, 0x40020c000054c8b0ull, 0xa060200202932a0ull, 0x1129918501408810ull, 0x80a1631d00201802ull, 0x8002015004010100ull, 0x4140402080210ull, 0x2404290010040040ull, 0xa5020080880801ull, 0x204450221851280ull, 0x1180a0d0810c00ull, 0x40030c0b85000c04ull, 0x421660603000621ull, 0x41828d0000801ull, 0x1150020124000200ull, 0x8800094500b00400ull, 0x9045627846002100ull, 0x4083020241684393ull, 0xa060140d03009c0ull, 0x1102826505050801ull, 0x418840724554a104ull, 0x8030c0c1418028ull, 0x880304880004ull, 0x331408e39202ull, 0x2102853a280a000ull, 0x934a0a00c09000ull, 0xa04c508a02840ull, 0xa4208200d1a104ull, 0x400d14132814500ull, 0x400010c0c14180ull, 0x1068000208808ull, 0x40042281362c0ull, 0x40058007844621c0ull, 0x8a8098500c090ull, 0x4003440102023109ull});
    //     arrays.push_back({0x821518401104018ull, 0x100328c320b06014ull, 0x10418300a0620100ull, 0xa2aa0960010242ull, 0x810c050480428212ull, 0x700a293a8082000ull, 0x2090a4ca280001ull, 0x910a3064020800ull, 0x2008890304c29280ull, 0x42203065460b060ull, 0x302441818980500aull, 0x422aa01600010ull, 0x14491040a01400ull, 0x80100a254270080ull, 0x200080a0aa0a1340ull, 0x40200028926a0a00ull, 0x4000550901c100ull, 0x101c90090580c086ull, 0x208000400241207ull, 0x1920840802044000ull, 0x910110606100020ull, 0x1000201a14300ull, 0x1283804071430c00ull, 0x1c420b120c200ull, 0x10202020190602c0ull, 0x401010240d028193ull, 0x1204040382080110ull, 0x2608020a2020200ull, 0x1010001904000ull, 0x4101120013018080ull, 0x40c8060080526540ull, 0x8b20600242932c6ull, 0x242861a00212012ull, 0x2655284200808ull, 0x1000425000180020ull, 0x100a0080180081ull, 0x900108020080a200ull, 0x210a1200008800ull, 0x40930963a0020802ull, 0x815011108529140ull, 0x3030506004001ull, 0x2030a18603082000ull, 0x84101828d0101800ull, 0x450420204a022ull, 0x4000194d00b00400ull, 0x2028181a2000100ull, 0x441c221530384ull, 0x44c60188b03009c0ull, 0x121328a0309020cull, 0x20009a8281448000ull, 0x142040c0c1418221ull, 0x8000100084044000ull, 0x81011281362c004ull, 0x410427140280a010ull, 0x280a04e1104920ull, 0x420a04c500982840ull, 0x800019010111a100ull, 0xe20009945024280ull, 0x20c461814181ull, 0x28020503411082ull, 0x85090120281362c0ull, 0x44200414090281c0ull, 0x13142500c090ull, 0xd1404c286003024ull});
    //     arrays.push_back({0x821518401104018ull, 0xa0032c2180a4c000ull, 0x804181898051000aull, 0x3022aa0160a00000ull, 0x404a008000404ull, 0xa25427000100ull, 0x612509a80040ull, 0x40010a3064020804ull, 0x2060902a4e50140ull, 0x4000130880c10048ull, 0x5900418181805012ull, 0x400022aa01614102ull, 0x4140422020000ull, 0x820180a253a80028ull, 0x2084126444030850ull, 0x420044852651407ull, 0x870ca1070c056600ull, 0x1cc003060180a0ull, 0x1d0000800d83014ull, 0x1008080082044104ull, 0x62100401040810ull, 0x4202000058050942ull, 0x10c00073055800ull, 0x201c2003191a280ull, 0x2020401b448300ull, 0x40820002d229188ull, 0x20410090090602ull, 0x50240080401020ull, 0x808010082000ull, 0x410002100203ull, 0x8020403b54b00ull, 0x410101602c28c0ull, 0x10305ba04202004ull, 0x2601430d01208822ull, 0x20a14410000a0020ull, 0x200800010811ull, 0x2140004010070100ull, 0x81040801200a1004ull, 0x4c2a861040400ull, 0xc20140c0d0020200ull, 0x8182831a003800ull, 0x300610589002010ull, 0x400890892040902ull, 0x10824e4208000180ull, 0x2c00080100441400ull, 0x91030141aa000100ull, 0xc031140604c10ull, 0x10460148c0303601ull, 0x8031328a028a0001ull, 0x123051818c032020ull, 0x4420006181418080ull, 0x4408000020880a08ull, 0x400a281362c402ull, 0x80027942280a008ull, 0x40280a04c0a0a111ull, 0x28304210221312cull, 0x2480310111a100ull, 0x401014142614500ull, 0x5001422861814180ull, 0x1000420206ull, 0x400000021142400ull, 0x24020050272481c0ull, 0x1182080801844265ull, 0x1001a200823108ull});
    //     arrays.push_back({0x821518401104018ull, 0x452488200803c80ull, 0x10418180a0c00020ull, 0x2240d0600400000ull, 0x140510c00c409000ull, 0x2001c28a92204001ull, 0x8606646460a0004ull, 0x82813a1104020700ull, 0x7000221342020060ull, 0x3030600a0c8ull, 0x11418140c0c000ull, 0x522aa01620c00ull, 0x6042420000404ull, 0x402001c28a120000ull, 0x104825442039000ull, 0x74011262218401ull, 0x438230505c60180ull, 0x209003061180a0ull, 0x8007000d02818ull, 0x2009080820420040ull, 0x8102002400a20004ull, 0x202004422012005ull, 0x840a0061828c00ull, 0x1c04048626524ull, 0x14104001050a84c8ull, 0x4a8a0104a0280ull, 0x240900088004410ull, 0x401080084004010ull, 0x41010000104008ull, 0x808020034220528ull, 0x20400044c98e2ull, 0x144030040d8c124ull, 0x1003051a02401000ull, 0x8b0c2a0100402ull, 0x21040404a080200ull, 0x810020080080080ull, 0x5021080200402200ull, 0x22200500c2020804ull, 0xc0830141a10a0800ull, 0x4008a43830010100ull, 0x402c65806020c00ull, 0xc01186054380c601ull, 0x8080182cc8003000ull, 0x8804022018000102ull, 0x10100200608204ull, 0x41a06051004281ull, 0x546218060c408ull, 0x1a10160940200ull, 0x6101350503092000ull, 0x6908388208928000ull, 0x520c0c1418140ull, 0x300040084040400ull, 0x800220d20860802ull, 0x227540240a080ull, 0x808018442740aull, 0x5218028300c03044ull, 0x20806411a140ull, 0x20000a135014488ull, 0x8210000c0c14180ull, 0x800800208844ull, 0x24000aa81408e390ull, 0x3527540240a0ull, 0x80c100382044254ull, 0x1080800d08a1158ull});
    //     arrays.push_back({0x200264014280a04dull, 0x1212488100788002ull, 0x28c19140c0c01c20ull, 0xc040086080000ull, 0x10041c2000000180ull, 0x1c28a92000000ull, 0x806066028c8c0100ull, 0x8008c4c96101c000ull, 0x22611c1810100ull, 0x8530861010034ull, 0xc01218740a06000ull, 0x10a8040400824000ull, 0x2022b1040400208ull, 0x2000028660a01230ull, 0x3000066028c0c20ull, 0x502091261018808ull, 0xa4402060c028182ull, 0x402001c905810066ull, 0x24049208001900ull, 0x1008001262004000ull, 0x20801c00a00000ull, 0x40042006028c2000ull, 0x4048162418406ull, 0x802000030c30288ull, 0x860200a990602e2ull, 0x41010008d030140ull, 0x108020063040b08ull, 0x8038020280200a0ull, 0x8015404104010040ull, 0x1113021008080400ull, 0x804040051a14300ull, 0x1088212a1010081ull, 0x808d28508502000ull, 0x2004100446090900ull, 0x4110202420180800ull, 0x200a01800c50104ull, 0x641c180200842008ull, 0x7010004080211008ull, 0xd118010104804800ull, 0x80050202000090c0ull, 0x84a8e240121002ull, 0x410061048b00a000ull, 0x86001208000480ull, 0x12860c208006080ull, 0x4202098500b00400ull, 0x2040608871048080ull, 0x30202c5200402ull, 0x4002a308c0310200ull, 0x3030305060000ull, 0x4008309209910000ull, 0x7800092220d08208ull, 0x4202010aull, 0x200108405040128ull, 0xd1a040130a0160c4ull, 0x30860802044a621aull, 0x6028328699001ull, 0x204240062d00900ull, 0xa100008080e89303ull, 0x250008092220d080ull, 0x1100002010218800ull, 0x8882002012020210ull, 0x101422554102160ull, 0x20b0c0c019862ull, 0x401900120b100ull});
    //     arrays.push_back({0x40100401004010ull, 0x130880c1004000ull, 0x4080200400000ull, 0x9040100000000ull, 0x2021000000000ull, 0x901008000000ull, 0x1040162c30000ull, 0x242538054000ull, 0x3608c1841080ull, 0x2001826200a8ull, 0x40102020000ull, 0x80841000000ull, 0x11040000000ull, 0x20151a50000ull, 0x16221e00800ull, 0x30c302a000ull, 0x40002008010100ull, 0x2001002080100ull, 0x8001000202020ull, 0x10400808102000ull, 0x4000220a00000ull, 0x200210100800ull, 0x4000041041000ull, 0x200200820800ull, 0x4040020081000ull, 0x4020020080100ull, 0x4100002002040ull, 0x400200400a080ull, 0x4082004002000ull, 0x4004008080200ull, 0x2020004010100ull, 0x2002000808800ull, 0x4202000040400ull, 0x841000200200ull, 0x405000080020ull, 0x80800020a00ull, 0x20008480040020ull, 0x4080090020040ull, 0x2020200004800ull, 0x2008200010040ull, 0x4100804000800ull, 0x2008420000400ull, 0x2010404400200ull, 0x2028000420ull, 0x2000a0800400ull, 0x8101000300080ull, 0x9854400500400ull, 0x2080041000080ull, 0xa18603400000ull, 0x3888044b0000ull, 0x2084100000ull, 0x84040000ull, 0x420820000ull, 0xb0606022000ull, 0x20060287006000ull, 0x9450800983000ull, 0x340401460300ull, 0x8071085300ull, 0x42009000ull, 0x840400ull, 0x4208200ull, 0x280a098160ull, 0x200810e8a048ull, 0x40170904c2262dull});
    //     arrays.push_back({0x200264014280a04dull, 0x80110980c1004500ull, 0x8418180a0c02408ull, 0x2404070200258100ull, 0x1004242001802002ull, 0x40c1c28994000000ull, 0x22644188080200ull, 0x100486310060500ull, 0x80522530400c08aull, 0x10004441722a3920ull, 0x22104102012485ull, 0xc0422800008ull, 0x41084840840000ull, 0x406030188404048ull, 0x60012ac2034000ull, 0x400020061660600ull, 0x34081a15501d80ull, 0xa0000b84e0a080ull, 0x201000216090200cull, 0x80001820040a5ull, 0x8088804402a00000ull, 0x1010809011025ull, 0x31000a00b16148ull, 0x4220008b120c220ull, 0x2020229905831cull, 0x142000028a6400ull, 0x605c0a0004080050ull, 0x94040086401180ull, 0x90101041004002ull, 0x3050010000804100ull, 0x4002851002150480ull, 0x201106209018804ull, 0x24a06004141420ull, 0xb2c300484182100ull, 0x8080c230009808c1ull, 0x800040400080120ull, 0x6005100400008060ull, 0x960080110188040ull, 0x42020200045800ull, 0x8102040024004a00ull, 0xe20b52064005ull, 0x60a1884d00a00aull, 0x4080220030001a00ull, 0x10000c200802800ull, 0x800002120a044401ull, 0x3001200181004080ull, 0x4050601a0608400ull, 0x204970a000100ull, 0xc400620213225021ull, 0x40860a30c430007ull, 0x8104d04062458140ull, 0xa048800242020222ull, 0x801010020e014dull, 0x880208020900ull, 0x141409a240a40001ull, 0x203020241620480ull, 0x8042304c03200342ull, 0x880083901085280ull, 0xa1220841001ull, 0x1008004608804ull, 0x10034a81c0028604ull, 0x42004027942220a2ull, 0x80400302424168ull, 0x4003014400c018ull});

    //     uint64_t magics[64];
    //     uint32_t totalSize;
    //     concat_magics(arrays, magics, totalSize, false);
    //     fprintMagics(file, magics, NULL, totalSize, false);
    // }
    // {
    //     std::vector<std::vector<uint64_t>> arrays;
    //     arrays.push_back({0x80004000108020ull, 0x4008c410002000ull, 0x82000c884040820ull, 0x4420080402020421ull, 0x4200020060385004ull, 0x2001c0810020001ull, 0xa80010022000880ull, 0x2800041000a2080ull, 0x800320400080ull, 0x122402010102040ull, 0x2820016004c8056ull, 0x406000a0046204cull, 0x6001a200a000cull, 0x2001d05060010ull, 0x2204c80810204ull, 0x860010a0646482ull, 0x9801040002000c0ull, 0x80a2010102040ull, 0x9601030010442004ull, 0x4000090010002102ull, 0x628008004004880ull, 0x101010004000802ull, 0x12000400014948d0ull, 0x1408600004500a4ull, 0x238480004000ull, 0x40002000c002d000ull, 0x11308200420020ull, 0x804ad00100082300ull, 0x110100080004ull, 0x2402000200048831ull, 0x210222400411008ull, 0x8400c8200064104ull, 0x4040028020800c40ull, 0x800208501004001ull, 0x4406002082001542ull, 0x402202002830ull, 0x880080800400ull, 0x400800400802a00ull, 0x802011004000288ull, 0xc8402001151ull, 0x1800800140008020ull, 0x2020101034c000ull, 0x20081c0120020ull, 0x21000a1010008ull, 0x40202040a0a0020ull, 0x1201040002008080ull, 0x1808218a040010ull, 0x8010008044020011ull, 0x480202018401840ull, 0x400030b09300ull, 0x20002a00306b1600ull, 0x20002600290c9200ull, 0x802020408121200ull, 0x280040003050300ull, 0x50c8c941500400ull, 0x2008404030801840ull, 0x200a05087004202ull, 0xd010031802a026aull, 0x8002002680280a12ull, 0xc400a001806ull, 0x40c8000300040b29ull, 0x220a004c18151002ull, 0x640018810016344ull, 0x52091080c1040226ull});
    //     arrays.push_back({0x80061020804002ull, 0x1040009000422008ull, 0x100090040102000ull, 0x2080180080500024ull, 0x1020192202010020ull, 0xea00082405100200ull, 0xc00009411100812ull, 0x80010000364080ull, 0x4006800240058028ull, 0x22402010102040ull, 0x85020013e2408200ull, 0x206000a0a064060ull, 0x2001212020408ull, 0x20a001002000804ull, 0x4008404080810221ull, 0x8810200338404020ull, 0x2404622008100820ull, 0x10880a2010102041ull, 0x820206001080a0c0ull, 0x2808010040802ull, 0x8000808004008800ull, 0x400400401a004100ull, 0x4040018010210ull, 0xc020004874c01ull, 0x8000209280024000ull, 0x804420028040008cull, 0x8000910100402000ull, 0x50100080080383ull, 0x100180080040280ull, 0x959004900220400ull, 0x60100400060823ull, 0x90008a001ac104ull, 0x8840018021800440ull, 0x10002002400848ull, 0x2200480801000ull, 0x1080d800801000ull, 0x6180041101001800ull, 0x5240800400804200ull, 0x800200804300ull, 0x21510021810001c6ull, 0x400902240008000ull, 0xa0201010314000ull, 0x2002088220420010ull, 0xa01a00c121120008ull, 0x410202040a0a0021ull, 0x8010060004008080ull, 0x40011810140002ull, 0x20028c00c88a0001ull, 0x400420030b09200ull, 0x3809a00a8290200ull, 0x8e0092441200ull, 0x10002600280a1200ull, 0x4090060008161200ull, 0x4018040003030500ull, 0x8f009113400ull, 0x200000a304004200ull, 0x1990052004c2416ull, 0x8082002b003266ull, 0x126012812800aull, 0x6000640081216ull, 0x321800100500060dull, 0x210200100814010aull, 0x800100148018344ull, 0xb020010042340082ull});
    //     arrays.push_back({0x80004000108020ull, 0x4008100044a000ull, 0x1080100080082004ull, 0x100050010000820ull, 0x80020400880080ull, 0x500080400c50002ull, 0x1080020000800100ull, 0xd000100002c4086ull, 0x600800080604012ull, 0x4014402010102040ull, 0x2001042220080ull, 0x4402000a0047204aull, 0x2003a200a000cull, 0x4002001d15160010ull, 0x5208403100804041ull, 0x120000c0820904ull, 0x1408a08014804000ull, 0x1000c020004000ull, 0x4120040a20080ull, 0x690020100100ull, 0x20110008010035ull, 0x400801200c1040ull, 0x8000040088100102ull, 0x2240820001510084ull, 0x8800100210040ull, 0x200940005000ull, 0x8001804200201200ull, 0x80e010010021002aull, 0x9003000500080011ull, 0x1000900040042ull, 0x2420010400100208ull, 0x480011200004184ull, 0x80400a8001b0ull, 0x10042001c04000ull, 0xd00580802000ull, 0xae80090421001000ull, 0x440180025001100ull, 0x4801020080804400ull, 0x53004000208ull, 0x4841186000051ull, 0x10088a04000800aull, 0x22020101036c000ull, 0x520001002828020ull, 0xa47001000a10058ull, 0x142202040a0a0020ull, 0x82010448020030ull, 0x128031006840008ull, 0x81610860820004ull, 0x100880030828c100ull, 0x1009e00a8502200ull, 0x801000600180ull, 0x10c026002c8a1200ull, 0x2002020408121200ull, 0x28040003050300ull, 0x20020001c241c0ull, 0x8008010100e120e0ull, 0x4022018102204812ull, 0x200112010a80564aull, 0x280804a004758d2ull, 0x20100063006505ull, 0x4008128800112301ull, 0x2002108500402ull, 0x1100188011374ull, 0x20204400810222ull});
    //     arrays.push_back({0x80004000108020ull, 0x4a40002000c83001ull, 0x20802000801a1000ull, 0x300081001002620ull, 0x200082004100a00ull, 0x2000b08100ca200ull, 0x80008001000200ull, 0xc180030000402280ull, 0x486d8001400080a8ull, 0x9020022004a8101ull, 0x100801008a00080ull, 0x5022004842009020ull, 0x7000800100500ull, 0x12002814900200ull, 0x2000a00885401ull, 0x1c2000114840452ull, 0x4040028020800c40ull, 0x1010040008420ull, 0x470020010010ull, 0x40a020011200840ull, 0x802020004682010ull, 0x7000808002000401ull, 0x600140001102208ull, 0xa20001810274ull, 0x1280004040002000ull, 0x60a0004040013002ull, 0x401200080300280ull, 0x1410010100082110ull, 0xb40150100308800ull, 0x2808020080040080ull, 0xd0500400080906ull, 0x4822800180004100ull, 0x492800301002240ull, 0x1000810606002040ull, 0x400409105002000ull, 0x408280482801000ull, 0x441800801804400ull, 0x20004a2001018ull, 0x3042091064001802ull, 0x90000249a2000304ull, 0x400080238000ull, 0x202010103c4000ull, 0x240402001050010ull, 0x50041200200a0040ull, 0x2007000608010010ull, 0x1090020004008080ull, 0x6244021011040048ull, 0x488064020009ull, 0x80202010401840ull, 0x84000403890b100ull, 0x2000100020028280ull, 0x8282600140a00ull, 0xa010180a080c0600ull, 0x20040003050300ull, 0x441081005020400ull, 0x4008010000e820e0ull, 0x4022002011004082ull, 0xc0008f0040009111ull, 0x4000389020010041ull, 0x90204600480a2006ull, 0x109000430480003ull, 0x1000894000201ull, 0x201106209018804ull, 0x6410092091004402ull});
    //     arrays.push_back({0x80004004a01084ull, 0x40094060001002ull, 0x2000a0052408020ull, 0x100209000490024ull, 0x80020800800400ull, 0x200040118500a00ull, 0x1080038001000200ull, 0x1100045382a10002ull, 0x80800024804000ull, 0x1000802002400680ull, 0x6967001142200100ull, 0x1004805000807800ull, 0x102001212020408ull, 0x200020088f014ull, 0x4000ec2c30810ull, 0x9580802480004100ull, 0x5140088000204690ull, 0x950044001600040ull, 0x548101002002c010ull, 0x5088008010040880ull, 0x5010010080044ull, 0x200101000e180400ull, 0x8504808001001200ull, 0x10120000850264ull, 0x140002080008050ull, 0x3100210100400084ull, 0x8000100080200080ull, 0x89000a0200304020ull, 0x8c0080080080ull, 0x2001001900024400ull, 0x188104400030688ull, 0x440200108041ull, 0x5080004000402001ull, 0x41004001006080ull, 0x210002001010040ull, 0x5002409001000ull, 0xa802800800400ull, 0x41040080800200ull, 0x80510884000230ull, 0x2004092000c21ull, 0x4000804201060020ull, 0x1019402010084000ull, 0x1108200220040ull, 0xc002600402a000eull, 0x10020020060e000aull, 0x485000234010018ull, 0x11140200010100ull, 0x4024014b05820004ull, 0x40a481020200ull, 0x1810102040201440ull, 0x6520004d00581700ull, 0xe3012046004a0a00ull, 0x2020408121200ull, 0x84000500063d00ull, 0x40088a70010400ull, 0x41840108c200ull, 0x561008000916045ull, 0x4400804000182103ull, 0x84004a002080460aull, 0x1a8080600409316ull, 0x2011008200402ull, 0x81000400081241ull, 0xa010080200900124ull, 0x120004504068062ull});
    //     arrays.push_back({0x4e80008040005063ull, 0x40001000200a41ull, 0x2200206a0010c080ull, 0x4a00101840220014ull, 0xc100080010050002ull, 0x1000100e6040008ull, 0x2080008006000900ull, 0x2000c04e3c28102ull, 0x80800032400088ull, 0x2031002100804010ull, 0x10010a0084100ull, 0x120012200a0040ull, 0x600100110008000dull, 0x4002000408460010ull, 0x4012000e00544843ull, 0x1800080004100ull, 0x83812080048a4000ull, 0xa0d1010020401080ull, 0x204808010082000ull, 0x808008010008008ull, 0x1008010011000804ull, 0x2808004000200ull, 0x3041010100040200ull, 0x888020000410094ull, 0xc00400180048020ull, 0x4240500040022008ull, 0x200800c0100340ull, 0x4100ca0200401020ull, 0x1000040080080080ull, 0x21202200802c0080ull, 0x1a8a0400810810ull, 0x28200025114ull, 0x4400304000800288ull, 0x62a040e009401000ull, 0x2000200089801000ull, 0x2024800800801000ull, 0x8004100801000500ull, 0x800200802400ull, 0x8100804004102ull, 0x30100440a000085ull, 0x200a800840008020ull, 0x1420100028444000ull, 0x280c222020010ull, 0x203001010028ull, 0x5004028018008004ull, 0x9a005044020009ull, 0x400600188c020005ull, 0x18041120004ull, 0x4000410030800100ull, 0x30004013200040ull, 0x2410b60021100ull, 0x110010804004040ull, 0x900080280540080ull, 0x1010084000e0900ull, 0x82000811040200ull, 0x8000010c80440200ull, 0x9ac4100d9820022ull, 0x4000284000108101ull, 0x11004408200011ull, 0x1002010008409ull, 0xa001048204406ull, 0x9002000324085022ull, 0x20280170018314ull, 0x20aa3c00e0c30586ull});
    //     arrays.push_back({0x80088040001020ull, 0x240004030012002ull, 0x80088210042000ull, 0x6080043000811800ull, 0x500030010180004ull, 0x4500128400050008ull, 0x1100408412000500ull, 0x200008a04c02401ull, 0x2800040002091ull, 0x8401000200342ull, 0x800a006200c01080ull, 0x41e002041100a00ull, 0x6101800400480081ull, 0x1004800400808200ull, 0xd000100420014ull, 0x1013000500044082ull, 0x100828000254008ull, 0x810020400100ull, 0x80e0808010062000ull, 0x12020010404820ull, 0x2450008001100ull, 0x1202008100800400ull, 0x5002440010110208ull, 0x2408220000440081ull, 0x4200401080002080ull, 0x841a000c0100a40ull, 0x5209004300200190ull, 0x80c86202000a3040ull, 0x401000500100800ull, 0xa500020080040080ull, 0x800028400100108ull, 0x8004800080204100ull, 0x8810c000e0800081ull, 0x2802802101004000ull, 0x2002200011004105ull, 0x400840800801002ull, 0x2800980080800400ull, 0x200800600800400ull, 0x85100804020200ull, 0x20008008d0800100ull, 0x2808a4000328000ull, 0x20101a42a0004001ull, 0xa920180220040ull, 0x9500008008080ull, 0x4c000800808004ull, 0x881a008004008002ull, 0x6211038040006ull, 0x8100014081000aull, 0x202800103402100ull, 0x1c83024202812a00ull, 0x1400c20280902200ull, 0x408028008100080ull, 0x40800800040080ull, 0x40100020c004900ull, 0x4141080201100400ull, 0x84400871600ull, 0x8250040800011ull, 0x40008020409101ull, 0x2012104028802202ull, 0x4110210074100009ull, 0x1004408000211ull, 0x1023000264002801ull, 0x54100008e005403ull, 0x2002010402204086ull});
    //     arrays.push_back({0x80800480400030a2ull, 0x240004030012002ull, 0x80088210042000ull, 0x6080043000811800ull, 0x1a80224800804400ull, 0x4500128400050008ull, 0x1100408412000500ull, 0x200008a04c02401ull, 0x2800040002091ull, 0x8401000200342ull, 0x800a006200c01080ull, 0x41e002041100a00ull, 0x6101800400480081ull, 0x1004800400808200ull, 0xd000100420014ull, 0x3010200320404020ull, 0x100828000254008ull, 0x810020400100ull, 0x80e0808010062000ull, 0x12020010404820ull, 0x2450008001100ull, 0x1202008100800400ull, 0x5002440010110208ull, 0x2408220000440081ull, 0x4200401080002080ull, 0x841a000c0100a40ull, 0x5209004300200190ull, 0x80c86202000a3040ull, 0x401000500100800ull, 0xa500020080040080ull, 0x800028400100108ull, 0x8004800080204100ull, 0x8810c000e0800081ull, 0x2802802101004000ull, 0x2002200011004105ull, 0x400840800801002ull, 0x2800980080800400ull, 0x200800600800400ull, 0x85100804020200ull, 0xd005000081000242ull, 0x80c000248010ull, 0x40a01000414000ull, 0xa920180220040ull, 0x8000401022020028ull, 0x4c000800808004ull, 0x881a008004008002ull, 0x6211038040006ull, 0x5080008420420005ull, 0x202800103402100ull, 0x1c83024202812a00ull, 0x1400c20280902200ull, 0x2c104201c8a200ull, 0x2001005008000d00ull, 0x40100020c004900ull, 0x6004c08014200ull, 0x84400871600ull, 0x8250040800011ull, 0x40008020409101ull, 0x2012104028802202ull, 0x2000900084210029ull, 0x1004408000211ull, 0x1023000264002801ull, 0x54100008e005403ull, 0x2002010402204086ull});
    //     uint64_t magics[64];
    //     uint32_t totalSize;
    //     concat_magics(arrays, magics, totalSize, true);
    //     fprintMagics(file, magics, NULL, totalSize, true);
    // }
    // fclose(file);

    // uint32_t timelimit = 60000;

    // for (uint32_t runIdx = 3; timelimit >= 2000; timelimit /= 2, runIdx++) {
    //     FILE* file = fopen(("search_runs/run" + std::to_string(runIdx) + ".txt").c_str(), "w");
    //     searchBestSizeTableMagics(false, timelimit, file, runIdx);
    //     searchBestSizeTableMagics(true, timelimit, file, runIdx);
    //     fclose(file);
    // }

    // PCG32* rng = createPCG32();
    // pcg32Seed(rng, pcgHash(0x445020000404cull), pcgHash(square));
    // uint32_t totalSize, bestSize, unused;
    // uint64_t magic;
    // magic = findBishopMagic(square, rng, &bestSize, &unused, 0);
    // printf("0x%llxull - Size: %u - Unused bits: %u\n", magic, bestSize, unused);
    // while (true) {
    //     magic = findBishopMagic(square, rng, &totalSize, &unused, 0);
    //     if (totalSize < bestSize) {
    //         bestSize = totalSize;
    //         printf("0x%llxull - Size: %u - Unused bits: %u\n", magic, bestSize, unused);
    //     }
    //     if (unused >= 1) {
    //         break;
    //     }
    // }
    // printf("0x%llxull - Size: %u - Unused bits: %u\n", magic, totalSize, unused);
    // destroyPCG32(rng);

    // auto startTime = std::chrono::high_resolution_clock::now();
    // bishopMagicSearch(magics, rng, &bestSize);
    // printMagics(magics, NULL, bestSize, false);
    // for (uint32_t i = 0; i < 1000; i++) {
    //     bishopMagicSearch(magics, rng, &totalSize);
    //     if (totalSize < bestSize) {
    //         bestSize = totalSize;
    //         printMagics(magics, NULL, bestSize, false);
    //     }
    // }
    // std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - startTime;
    // std::cout << "Took " << elapsed.count() / 1000.0f << " seconds\n";
    return 0;
}
