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
#include "slab.h"
#include "bitmap_allocator.h"
#include "ut_platform.h"

namespace Mark3 {

#define DEFAULT_SLAB_SIZE (256)
#define DEFAULT_SLAB_COUNT (8)
#define DEFAULT_ALLOC_SIZE  (16)

extern "C" {
void __cxa_guard_acquire() {};
void __cxa_guard_release() {};
}

namespace {
K_WORD awSlabMem[(DEFAULT_SLAB_COUNT * DEFAULT_SLAB_SIZE) / sizeof(K_WORD)];
uint8_t* pAllocs[(DEFAULT_SLAB_COUNT * DEFAULT_SLAB_SIZE)/ DEFAULT_ALLOC_SIZE];
Slab clSlab;
static BitmapAllocator clAllocator;
} // anonymous namespace

class IUT {
public:
    static Slab* build() {
        clAllocator.Init(awSlabMem, sizeof(awSlabMem), DEFAULT_SLAB_SIZE);

        static auto allocPage = [](uint32_t* pu32PageSize_) {
            *pu32PageSize_ = DEFAULT_SLAB_SIZE;
            void* rc = nullptr;
            rc = clAllocator.Allocate(nullptr);
            return rc;
        };

        static auto freePage = [](void* pvPage_) {
            clAllocator.Free(pvPage_);
        };

        clSlab.Init(DEFAULT_ALLOC_SIZE, allocPage, freePage);
        return &clSlab;
    }

    static int getCapacity() {
        int capacity = 0;
        while (1) {
            pAllocs[capacity] = reinterpret_cast<uint8_t*>(clSlab.Alloc());
            if (!pAllocs[capacity]) {
                break;
            }
            capacity++;
        }

        for (int i = 0; i < capacity; i++) {
            clSlab.Free(pAllocs[i]);
            pAllocs[i] = nullptr;
        }
        return capacity;
    }
};

//---------------------------------------------------------------------------
TEST(ut_slab_default_init_pass)
{
    auto* iut = IUT::build();
    auto freePages = iut->GetFreePageCount();
    auto fullPages = iut->GetFullPageCount();

    EXPECT_TRUE(freePages == 0);
    EXPECT_TRUE(fullPages == 0);
}

//---------------------------------------------------------------------------
TEST(ut_slab_get_obj_size_pass)
{
    auto* iut = IUT::build();

    EXPECT_EQUALS(iut->GetObjSize(), DEFAULT_ALLOC_SIZE);
}

//---------------------------------------------------------------------------
TEST(ut_slab_page_count_pass)
{
    auto* iut = IUT::build();
    auto capacity = IUT::getCapacity();

    auto lastFreeCount = iut->GetFreePageCount();
    auto lastFullCount = iut->GetFullPageCount();

    auto freeCountChange = 0;
    auto fullCountChange = 0;

    for (int i = 0; i < capacity; i++) {
        pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Alloc());

        auto currFreeCount = iut->GetFreePageCount();
        auto currFullCount = iut->GetFullPageCount();

        if (lastFreeCount != currFreeCount) {
            freeCountChange++;
        }
        if (lastFullCount != currFullCount) {
            fullCountChange++;
        }
        lastFreeCount = currFreeCount;
        lastFullCount = currFullCount;
    }

    // Expect to allocate a slab page n-1 times (1 page used for metadata)
    // The number of times the full-count changes is 2 * full-count changes
    // (it toggles to 0 when a page moves to the "full" list, and 1 when a
    // new slab page is brought in on the next allocation)
    EXPECT_TRUE(fullCountChange == (DEFAULT_SLAB_COUNT - 1));
    EXPECT_TRUE(freeCountChange == (fullCountChange * 2));

    // When we free half of the allocations, there should be *no* full
    // pages, and *only* slabs-with-free-data pages.
    for (int i = 0; i < capacity; i += 2) {
        iut->Free(pAllocs[i]);
    }

    EXPECT_TRUE(iut->GetFullPageCount() == 0);
    EXPECT_TRUE(iut->GetFreePageCount() == (DEFAULT_SLAB_COUNT - 1));

    // Free the rest of the allocations, verify no slab pages are in use
    for (int i = 1; i < capacity; i += 2) {
        iut->Free(pAllocs[i]);
    }

    EXPECT_TRUE(iut->GetFullPageCount() == 0);
    EXPECT_TRUE(iut->GetFreePageCount() == 0);
}

//---------------------------------------------------------------------------
TEST(ut_slab_double_free_pass)
{
    auto* iut = IUT::build();
    pAllocs[0] = reinterpret_cast<uint8_t*>(iut->Alloc());
    pAllocs[1] = reinterpret_cast<uint8_t*>(iut->Alloc());

    EXPECT_TRUE(iut->GetFreePageCount() == 1);

    // make sure double-freeing one alloc'd block doesn't cause the page count to
    // change

    iut->Free(pAllocs[0]);
    EXPECT_TRUE(iut->GetFreePageCount() == 1);

    iut->Free(pAllocs[0]);
    EXPECT_TRUE(iut->GetFreePageCount() == 1);

    iut->Free(pAllocs[1]);
    EXPECT_TRUE(iut->GetFreePageCount() == 0);

    iut->Free(pAllocs[1]);
    EXPECT_TRUE(iut->GetFreePageCount() == 0);
}

//---------------------------------------------------------------------------
TEST(ut_slab_alloc_free_pass)
{
    auto* iut = IUT::build();
    auto capacity = IUT::getCapacity();

    // run the test in multiple passes
    for (int j = 0; j < 10; j++) {
        // No unallocated or partially-allocated pages left at the beginning
        EXPECT_TRUE(iut->GetFreePageCount() == 0);
        EXPECT_TRUE(iut->GetFullPageCount() == 0);

        // Allocate until exhaustion
        for (int i = 0; i < capacity; i++) {
            pAllocs[i] = reinterpret_cast<uint8_t*>(iut->Alloc());
            EXPECT_TRUE(pAllocs[i] != nullptr);
        }
        EXPECT_TRUE(iut->Alloc() == nullptr);

        // No partially-allocated pages left at exhaustion, multiple allocated pages
        EXPECT_TRUE(iut->GetFreePageCount() == 0);
        EXPECT_TRUE(iut->GetFullPageCount() != 0);

        // Free all allocated objects
        for (int i = 0; i < capacity; i++) {
            iut->Free(pAllocs[i]);
            pAllocs[i] = nullptr;
        }

        // No unallocated or partially-allocated pages left at the end
        EXPECT_TRUE(iut->GetFreePageCount() == 0);
        EXPECT_TRUE(iut->GetFullPageCount() == 0);
    }
}

//---------------------------------------------------------------------------
//===========================================================================
// Test Whitelist Goes Here
//===========================================================================
TEST_CASE_START
TEST_CASE(ut_slab_default_init_pass),
TEST_CASE(ut_slab_get_obj_size_pass),
TEST_CASE(ut_slab_page_count_pass),
TEST_CASE(ut_slab_double_free_pass),
TEST_CASE(ut_slab_alloc_free_pass),
TEST_CASE_END
} // namespace mark3
