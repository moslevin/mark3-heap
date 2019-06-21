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
#include "bitmap_allocator.h"
#include "memutil.h"
#include "ut_platform.h"

#define DEFAULT_BITMAP_SIZE (256)
#define DEFAULT_ALLOC_SIZE  (16)

using namespace Mark3;
namespace {
K_WORD awBitmapData[DEFAULT_BITMAP_SIZE/sizeof(K_WORD)];
uint8_t* pAllocs[DEFAULT_BITMAP_SIZE/DEFAULT_ALLOC_SIZE];
} // anonymous namespace

namespace Mark3
{
class IUT {
public:
    static BitmapAllocator* build() {
        static BitmapAllocator clBitmap;
        clBitmap.Init(awBitmapData, sizeof(awBitmapData), DEFAULT_ALLOC_SIZE);
        return &clBitmap;
    }
};

//---------------------------------------------------------------------------
TEST(ut_bitmap_default_init_pass)
{
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    EXPECT_TRUE(u32Capacity > 0);
}

//---------------------------------------------------------------------------
TEST(ut_bitmap_alloc_exhaustive_pass)
{
    // Ensure we can allocate all of the objects stated in the allocator's
    // capacity, and that when allocation fails beyond that.
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    for (uint32_t i = 0; i < u32Capacity; i++) {
        EXPECT_TRUE(iut->GetNumFree() == (u32Capacity - i));
        pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
        EXPECT_TRUE(pAllocs[i] != nullptr);
    }

    EXPECT_TRUE(iut->GetNumFree() == 0);
    EXPECT_TRUE(iut->Allocate(nullptr) == nullptr);
    EXPECT_TRUE(iut->GetNumFree() == 0);
}

//---------------------------------------------------------------------------
TEST(ut_bitmap_is_full_pass)
{
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    for (uint32_t i = 0; i < u32Capacity; i++) {
        EXPECT_FALSE(iut->IsFull());
        pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
        EXPECT_TRUE(pAllocs[i] != nullptr);
        if (!pAllocs[i]) {
            return;
        }
    }
    EXPECT_TRUE(iut->IsFull());
}

//---------------------------------------------------------------------------
TEST(ut_bitmap_is_empty_pass)
{
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    EXPECT_TRUE(iut->IsEmpty());
    for (uint32_t i = 0; i < u32Capacity; i++) {
        pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
        EXPECT_TRUE(pAllocs[i] != nullptr);
        if (!pAllocs[i]) {
            return;
        }
        EXPECT_FALSE(iut->IsEmpty());
    }
    EXPECT_FALSE(iut->IsEmpty());
}

//---------------------------------------------------------------------------
TEST(ut_bitmap_double_free_handled)
{
    // Double-free should not cause free bit count to increment twice
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    for (uint32_t i = 0; i < u32Capacity; i++) {
        pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
        EXPECT_TRUE(pAllocs[i] != nullptr);
        if (!pAllocs[i]) {
            return;
        }
    }

    EXPECT_TRUE(iut->GetNumFree() == 0);
    iut->Free(pAllocs[0]);
    EXPECT_TRUE(iut->GetNumFree() == 1);
    iut->Free(pAllocs[0]);
    EXPECT_TRUE(iut->GetNumFree() == 1);
}

//---------------------------------------------------------------------------
TEST(ut_bitmap_block_write_pass)
{
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    // allocate all, write all blocks to full, free all blocks, repeat.
    // Don't free on repeat.  This test ensures we can exhaustively allocate
    // and use the pool, and when not writing out-of-bounds, we can free and
    // re-allocate each item.  Also verifies that the contents of the allocated
    // user-accessible data is not inadvertently overwritten during allocation/free.
    for (int k = 0; k < 2; k ++) {
        for (uint32_t i = 0; i < u32Capacity; i++) {
            pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
            EXPECT_TRUE(pAllocs[i] != nullptr);
            if (!pAllocs[i]) {
                return;
            }
            for (uint32_t j = 0; j < DEFAULT_ALLOC_SIZE; j++) {
                pAllocs[i][j] = i;
            }
        }
        if (k == 0) {
            for (uint32_t i = 0; i < u32Capacity; i++) {
                iut->Free(pAllocs[i]);
            }
        }
    }

    // Verify block contents are intact
    for (uint32_t i = 0; i < u32Capacity; i++) {
        uint8_t au8Data[DEFAULT_ALLOC_SIZE];
        for (uint32_t j = 0; j < DEFAULT_ALLOC_SIZE; j++) {
            au8Data[j] = i;
        }
        EXPECT_TRUE(MemUtil::CompareMemory(au8Data, pAllocs[i], DEFAULT_ALLOC_SIZE));
    }
}

//---------------------------------------------------------------------------
TEST(ut_bitmap_alloc_patterns_pass)
{
    auto* iut = IUT::build();
    auto u32Capacity = iut->GetNumFree();

    EXPECT_TRUE(iut->GetNumFree() == u32Capacity);
    for (int l = 0; l < 3; l++) {
        for (int k = 2; k < 7; k++) {
            // Allocate everything
            for (uint32_t i = 0; i < u32Capacity; i++) {
                pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
                EXPECT_TRUE(pAllocs[i] != nullptr);
                if (!pAllocs[i]) {
                    return;
                }
            }

            // Free in "bands", then reallocate in bands
            for (int j = 0; j < k; j++) {
                for (uint32_t i = j; i < u32Capacity; i += k) {
                    for (int m = 0; (m < l) && ((m + i) < u32Capacity) ; m++) {
                        iut->Free(pAllocs[i + m]);
                    }
                }
                for (uint32_t i = j; i < u32Capacity; i += k) {
                    for (int m = 0; (m < l) && ((m + i) < u32Capacity) ; m++) {
                        pAllocs[i + m] = reinterpret_cast<uint8_t*>(iut->Allocate(nullptr));
                        EXPECT_TRUE(pAllocs[i + m] != nullptr);
                        if (!pAllocs[i + m]) {
                            return;
                        }
                    }
                }
            }

            // Free all
            for (uint32_t i = 0; i < u32Capacity; i++) {
                iut->Free(pAllocs[i]);
            }
        }
    }
}

//---------------------------------------------------------------------------
//===========================================================================
// Test Whitelist Goes Here
//===========================================================================
TEST_CASE_START
TEST_CASE(ut_bitmap_default_init_pass),
TEST_CASE(ut_bitmap_alloc_exhaustive_pass),
TEST_CASE(ut_bitmap_is_full_pass),
TEST_CASE(ut_bitmap_is_empty_pass),
TEST_CASE(ut_bitmap_double_free_handled),
TEST_CASE(ut_bitmap_block_write_pass),
TEST_CASE(ut_bitmap_alloc_patterns_pass),
TEST_CASE_END
} // namespace mark3
