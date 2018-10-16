/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012 - 2018 m0slevin, all rights reserved.
See license.txt for more information
===========================================================================*/
#include "mark3.h"
#include "arena.h"
#include "bitmap_allocator.h"
#include "ut_platform.h"
#include "memutil.h"

namespace Mark3 {

extern "C" {
void __cxa_guard_acquire() {};
void __cxa_guard_release() {};
}

namespace {
#define SMALL_HEAP_TOTAL_SIZE (2048)
#define SMALL_HEAP_MAX_ALLOC_SIZE (256)
#define TOTAL_ALLOCATIONS (200)

uint8_t* pvAllocs[TOTAL_ALLOCATIONS];

K_WORD m_awHeapMem[SMALL_HEAP_TOTAL_SIZE / sizeof(K_WORD)];
K_ADDR m_awHeapSize[] = { 16, 24, 40, 64, 104, 168, SMALL_HEAP_MAX_ALLOC_SIZE };

Arena m_clArena;

} // anonymous namespace

class IUT {
public:
    static Arena* build() {
        m_clArena.Init(m_awHeapMem, sizeof(m_awHeapMem), m_awHeapSize, sizeof(m_awHeapSize) / sizeof(K_ADDR));

        return &m_clArena;
    }
    static uint32_t getNumFreeBlocks() {
        auto i = m_clArena.GetListCount();
        uint32_t freeBlocks = 0;
        for (auto j = 0; j < i; j++) {
            uint32_t blockSize;
            uint32_t blockCount;
            m_clArena.GetListInfo(j, &blockSize, &blockCount);
            freeBlocks += blockCount;
        }
        return freeBlocks;
    }
    static int getMemFree() {
        auto i = m_clArena.GetListCount();
        uint32_t freeMem = 0;
        for (auto j = 0; j < i; j++) {
            uint32_t blockSize;
            uint32_t blockCount;
            m_clArena.GetListInfo(j, &blockSize, &blockCount);
            freeMem += blockCount * blockSize;
        }
        return freeMem;
    }
};

//---------------------------------------------------------------------------
TEST(ut_arena_init_pass)
{
    auto iut = IUT::build();

    EXPECT_TRUE(iut != nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_arena_alloc_min_pass)
{
    auto iut = IUT::build();

    auto alloc = iut->Allocate(1);

    EXPECT_TRUE(alloc != nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_arena_alloc_max_pass)
{
    auto iut = IUT::build();

    auto alloc = iut->Allocate(SMALL_HEAP_MAX_ALLOC_SIZE);

    EXPECT_TRUE(alloc != nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_arena_alloc_too_big_fail)
{
    auto iut = IUT::build();

    auto alloc = iut->Allocate(SMALL_HEAP_MAX_ALLOC_SIZE + 1);

    EXPECT_TRUE(alloc == nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_arena_min_free_pass)
{
    auto iut = IUT::build();
    auto startMem = IUT::getMemFree();
    auto alloc = iut->Allocate(1);
    auto memAfterAlloc = IUT::getMemFree();

    EXPECT_TRUE((startMem - memAfterAlloc) >= 1);

    iut->Free(alloc);

    auto memAfterFree = IUT::getMemFree();

    EXPECT_EQUALS(startMem, memAfterFree);
}

//---------------------------------------------------------------------------
TEST(ut_arena_max_free_pass)
{
    auto iut = IUT::build();
    auto startMem = IUT::getMemFree();
    auto alloc = iut->Allocate(SMALL_HEAP_MAX_ALLOC_SIZE);
    auto memAfterAlloc = IUT::getMemFree();

    EXPECT_TRUE((startMem - memAfterAlloc) >= SMALL_HEAP_MAX_ALLOC_SIZE);

    iut->Free(alloc);

    auto memAfterFree = IUT::getMemFree();

    EXPECT_EQUALS(startMem, memAfterFree);
}

//---------------------------------------------------------------------------
TEST(ut_arena_middle_free_pass)
{
    auto iut = IUT::build();
    auto startMem = IUT::getMemFree();
    auto alloc = iut->Allocate(128);
    auto memAfterAlloc = IUT::getMemFree();

    EXPECT_TRUE((startMem - memAfterAlloc) >= 128);

    iut->Free(alloc);

    auto memAfterFree = IUT::getMemFree();

    EXPECT_EQUALS(startMem, memAfterFree);
}

//---------------------------------------------------------------------------
TEST(ut_arena_exhaust_alloc_pass)
{
    auto* iut = IUT::build();
    auto count = 0;
    while (IUT::getMemFree()) {
        iut->Allocate(1);
        count++;
    }
    EXPECT_TRUE(count <= TOTAL_ALLOCATIONS);
}

//---------------------------------------------------------------------------
TEST(ut_arena_alloc_patterns_pass)
{
    auto* iut = IUT::build();
    auto startMem = IUT::getMemFree();

    uint32_t count = 0;
    while (IUT::getMemFree()) {
        iut->Allocate(1);
        count++;
    }
    iut = IUT::build();

    for (int l = 0; l < 3; l++) {
        for (int k = 2; k < 7; k++) {
            EXPECT_EQUALS(startMem, IUT::getMemFree());

            // Allocate everything
            for (uint32_t i = 0; i < count; i++) {
                pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(1));
                EXPECT_TRUE(pvAllocs[i] != nullptr);
                if (!pvAllocs[i]) {
                    return;
                }
            }
            EXPECT_EQUALS(0, IUT::getMemFree());

            // Free in "bands", then reallocate in bands
            for (int j = 0; j < k; j++) {
                for (uint32_t i = j; i < count; i += k) {
                    for (int m = 0; (m < l) && ((m + i) < count) ; m++) {
                        iut->Free(pvAllocs[i + m]);
                    }
                }
                for (uint32_t i = j; i < count; i += k) {
                    for (int m = 0; (m < l) && ((m + i) < count) ; m++) {
                        pvAllocs[i + m] = reinterpret_cast<uint8_t*>(iut->Allocate(1));
                        EXPECT_TRUE(pvAllocs[i] != nullptr);
                        if (!pvAllocs[i]) {
                            return;
                        }
                    }
                }
                EXPECT_EQUALS(0, IUT::getMemFree());
            }

            // Free all
            for (uint32_t i = 0; i < count; i++) {
                iut->Free(pvAllocs[i]);
            }
        }
    }
}

//---------------------------------------------------------------------------
//===========================================================================
// Test Whitelist Goes Here
//===========================================================================
TEST_CASE_START
TEST_CASE(ut_arena_init_pass),
TEST_CASE(ut_arena_alloc_min_pass),
TEST_CASE(ut_arena_alloc_max_pass),
TEST_CASE(ut_arena_alloc_too_big_fail),
TEST_CASE(ut_arena_min_free_pass),
TEST_CASE(ut_arena_middle_free_pass),
TEST_CASE(ut_arena_max_free_pass),
TEST_CASE(ut_arena_exhaust_alloc_pass),
TEST_CASE(ut_arena_alloc_patterns_pass),
TEST_CASE_END
} // namespace mark3
