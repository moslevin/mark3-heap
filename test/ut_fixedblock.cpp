/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012 - 2019 m0slevin, all rights reserved.
See license.txt for more information
===========================================================================*/
#include "mark3.h"
#include "fixed_heap.h"
#include "ut_platform.h"
#include "memutil.h"

#define BLOCK_OVERHEAD (sizeof(K_ADDR) * 3)
#define BLOCK_COUNT (5)
#define BLOCK_SIZES (5)

#define BLOCK0_SIZE (4)
#define BLOCK0_COUNT (BLOCK_COUNT)
#define BLOCK0_HEAP_SIZE ((BLOCK0_SIZE + BLOCK_OVERHEAD) * BLOCK0_COUNT)

#define BLOCK1_SIZE (8)
#define BLOCK1_COUNT (BLOCK_COUNT)
#define BLOCK1_HEAP_SIZE ((BLOCK1_SIZE + BLOCK_OVERHEAD) * BLOCK1_COUNT)

#define BLOCK2_SIZE (16)
#define BLOCK2_COUNT (BLOCK_COUNT)
#define BLOCK2_HEAP_SIZE ((BLOCK2_SIZE + BLOCK_OVERHEAD) * BLOCK2_COUNT)

#define BLOCK3_SIZE (32)
#define BLOCK3_COUNT (BLOCK_COUNT)
#define BLOCK3_HEAP_SIZE ((BLOCK3_SIZE + BLOCK_OVERHEAD) * BLOCK3_COUNT)

#define BLOCK4_SIZE (64)
#define BLOCK4_COUNT (BLOCK_COUNT)
#define BLOCK4_HEAP_SIZE ((BLOCK4_SIZE + BLOCK_OVERHEAD) * BLOCK4_COUNT)

#define BLOCK5_SIZE (0)
#define BLOCK5_COUNT (0)

#define TOTAL_HEAP_SIZE \
    (BLOCK0_HEAP_SIZE + BLOCK1_HEAP_SIZE + BLOCK2_HEAP_SIZE + BLOCK3_HEAP_SIZE + BLOCK4_HEAP_SIZE)

#define TOTAL_ALLOCATIONS \
    (BLOCK0_COUNT + BLOCK1_COUNT + BLOCK2_COUNT + BLOCK3_COUNT + BLOCK4_COUNT)

using namespace Mark3;

extern "C" {
void __cxa_guard_acquire() {};
void __cxa_guard_release() {};
}

namespace {
K_WORD awHeap[TOTAL_HEAP_SIZE];

uint8_t* pvAllocs[TOTAL_ALLOCATIONS];
HeapConfig clHeapConfig[] = {
    { .m_uBlockSize = BLOCK0_SIZE, .m_uBlockCount = BLOCK0_COUNT },
    { .m_uBlockSize = BLOCK1_SIZE, .m_uBlockCount = BLOCK1_COUNT },
    { .m_uBlockSize = BLOCK2_SIZE, .m_uBlockCount = BLOCK2_COUNT },
    { .m_uBlockSize = BLOCK3_SIZE, .m_uBlockCount = BLOCK3_COUNT },
    { .m_uBlockSize = BLOCK4_SIZE, .m_uBlockCount = BLOCK4_COUNT },
    { .m_uBlockSize = 0},
};

FixedHeap clHeap;
} // anonymous namespace

namespace Mark3 {

class IUT {
public:
    static FixedHeap* build() {
        clHeap.Create(awHeap, clHeapConfig);
        return &clHeap;
    }
};

//---------------------------------------------------------------------------
TEST(ut_fixed_alloc_range_pass)
{
    auto iut = IUT::build();

    auto allocLow = iut->Allocate(1);
    auto allocHigh = iut->Allocate(BLOCK4_SIZE);
    auto allocOutOfBounds = iut->Allocate(BLOCK4_SIZE + 1);

    EXPECT_TRUE(allocLow != nullptr);
    EXPECT_TRUE(allocHigh != nullptr);
    EXPECT_TRUE(allocOutOfBounds == nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_fixed_exhaust_small_pass)
{
    auto iut = IUT::build();

    // exhaust the heap by allocating 1-byte blocks.  This will exhaust the
    // heaps from smallest to largest.
    for (int i = 0; i < TOTAL_ALLOCATIONS; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(1));
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }
    EXPECT_TRUE(iut->Allocate(1) == nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_fixed_exhaust_large_pass)
{
    auto iut = IUT::build();

    // exhaust the heap by allocating blocks from the largest size heap.  After
    // exhausting that size, the heap will return null/exhausted.
    for (int i = 0; i < BLOCK4_COUNT; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK4_SIZE));
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }

    EXPECT_TRUE(iut->Allocate(BLOCK4_SIZE) == nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_fixed_exhaust_exact_pass)
{
    auto iut = IUT::build();

    for (int i = 0; i < BLOCK4_COUNT; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK4_SIZE));
        MemUtil::SetMemory(pvAllocs[i], 0, BLOCK4_SIZE);
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }
    EXPECT_TRUE(iut->Allocate(BLOCK4_SIZE) == nullptr);
    for (int i = 0; i < BLOCK3_COUNT; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK3_SIZE));
        MemUtil::SetMemory(pvAllocs[i], 0, BLOCK3_SIZE);
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }
    EXPECT_TRUE(iut->Allocate(BLOCK3_SIZE) == nullptr);
    for (int i = 0; i < BLOCK2_COUNT; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK2_SIZE));
        MemUtil::SetMemory(pvAllocs[i], 0, BLOCK2_SIZE);
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }
    EXPECT_TRUE(iut->Allocate(BLOCK2_SIZE) == nullptr);
    for (int i = 0; i < BLOCK1_COUNT; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK1_SIZE));
        MemUtil::SetMemory(pvAllocs[i], 0, BLOCK1_SIZE);
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }
    EXPECT_TRUE(iut->Allocate(BLOCK1_SIZE) == nullptr);
    for (int i = 0; i < BLOCK0_COUNT; i++) {
        pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK0_SIZE));
        MemUtil::SetMemory(pvAllocs[i], 0, BLOCK0_SIZE);
        EXPECT_TRUE(pvAllocs[i] != nullptr);
    }
    EXPECT_TRUE(iut->Allocate(BLOCK0_SIZE) == nullptr);
}

//---------------------------------------------------------------------------
TEST(ut_fixed_alloc_free_pass)
{
    auto iut = IUT::build();

    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < BLOCK4_COUNT; i++) {
            pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK4_SIZE));
            EXPECT_TRUE(pvAllocs[i] != nullptr);
            if (!pvAllocs[i]) {
                break;
            }
            MemUtil::SetMemory(pvAllocs[i], 0, BLOCK4_SIZE);
        }
        EXPECT_TRUE(iut->Allocate(BLOCK4_SIZE) == nullptr);
        for (int i = 0; i < BLOCK3_COUNT; i++) {
            pvAllocs[i + 5] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK3_SIZE));
            EXPECT_TRUE(pvAllocs[i + 5] != nullptr);
            if (!pvAllocs[i + 5]) {
                break;
            }
            MemUtil::SetMemory(pvAllocs[i + 5], 0, BLOCK3_SIZE);
        }
        EXPECT_TRUE(iut->Allocate(BLOCK3_SIZE) == nullptr);
        for (int i = 0; i < BLOCK2_COUNT; i++) {
            pvAllocs[i + 10] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK2_SIZE));
            EXPECT_TRUE(pvAllocs[i + 10] != nullptr);
            if (!pvAllocs[i + 10]) {
                break;
            }
            MemUtil::SetMemory(pvAllocs[i + 10], 0, BLOCK2_SIZE);
        }
        EXPECT_TRUE(iut->Allocate(BLOCK2_SIZE) == nullptr);
        for (int i = 0; i < BLOCK1_COUNT; i++) {
            pvAllocs[i + 15] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK1_SIZE));
            EXPECT_TRUE(pvAllocs[i + 15] != nullptr);
            if (!pvAllocs[i + 15]) {
                break;
            }
            MemUtil::SetMemory(pvAllocs[i + 15], 0, BLOCK1_SIZE);
        }
        EXPECT_TRUE(iut->Allocate(BLOCK1_SIZE) == nullptr);
        for (int i = 0; i < BLOCK0_COUNT; i++) {
            pvAllocs[i + 20] = reinterpret_cast<uint8_t*>(iut->Allocate(BLOCK0_SIZE));
            EXPECT_TRUE(pvAllocs[i + 20] != nullptr);
            if (!pvAllocs[i + 20]) {
                break;
            }
            MemUtil::SetMemory(pvAllocs[i + 20], 0, BLOCK0_SIZE);
        }
        EXPECT_TRUE(iut->Allocate(BLOCK0_SIZE) == nullptr);
        for (int i = 0; i < TOTAL_ALLOCATIONS; i++) {
            iut->Free(pvAllocs[i]);
        }
    }
}

//---------------------------------------------------------------------------
TEST(ut_fixed_alloc_patterns_pass)
{
    auto* iut = IUT::build();

    for (int l = 0; l < 3; l++) {
        for (int k = 2; k < 7; k++) {
            // Allocate everything
            for (uint32_t i = 0; i < TOTAL_ALLOCATIONS; i++) {
                pvAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(1));
                EXPECT_TRUE(pvAllocs[i] != nullptr);
                if (!pvAllocs[i]) {
                    return;
                }
            }

            // Free in "bands", then reallocate in bands
            for (int j = 0; j < k; j++) {
                for (uint32_t i = j; i < TOTAL_ALLOCATIONS; i += k) {
                    for (int m = 0; (m < l) && ((m + i) < TOTAL_ALLOCATIONS) ; m++) {
                        iut->Free(pvAllocs[i + m]);
                    }
                }
                for (uint32_t i = j; i < TOTAL_ALLOCATIONS; i += k) {
                    for (int m = 0; (m < l) && ((m + i) < TOTAL_ALLOCATIONS) ; m++) {
                        pvAllocs[i + m] = reinterpret_cast<uint8_t*>(iut->Allocate(1));
                        EXPECT_TRUE(pvAllocs[i + m] != nullptr);
                        if (!pvAllocs[i + m]) {
                            return;
                        }
                    }
                }
            }

            // Free all
            for (uint32_t i = 0; i < TOTAL_ALLOCATIONS; i++) {
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
TEST_CASE(ut_fixed_alloc_range_pass),
TEST_CASE(ut_fixed_exhaust_small_pass),
TEST_CASE(ut_fixed_exhaust_large_pass),
TEST_CASE(ut_fixed_exhaust_exact_pass),
TEST_CASE(ut_fixed_alloc_free_pass),
TEST_CASE(ut_fixed_alloc_patterns_pass),
TEST_CASE_END
} // namespace mark3
