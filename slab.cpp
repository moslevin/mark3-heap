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
=========================================================================== */
/**
    @file slab.cpp
    @brief Slab allocator class implementation
*/

#include "slab.h"
#include "mark3.h"

namespace Mark3
{
//---------------------------------------------------------------------------
void SlabPage::InitPage(uint32_t u32PageSize_, uint32_t u32ObjSize_)
{
    LinkListNode::ClearNode();
    auto* pvBlock = reinterpret_cast<void*>((K_ADDR)this + sizeof(SlabPage));
    m_clAllocator.Init(pvBlock, u32PageSize_ - sizeof(SlabPage), u32ObjSize_);
}

//---------------------------------------------------------------------------
void* SlabPage::Alloc(void* pvTag_)
{
    return m_clAllocator.Allocate(pvTag_);
}

//---------------------------------------------------------------------------
void SlabPage::Free(void* pvObject_)
{
    m_clAllocator.Free(pvObject_);
}

//---------------------------------------------------------------------------
bool SlabPage::IsEmpty(void)
{
    return m_clAllocator.IsEmpty();
}

//---------------------------------------------------------------------------
bool SlabPage::IsFull(void)
{
    return m_clAllocator.IsFull();
}

//---------------------------------------------------------------------------
void Slab::Init(uint32_t u32ObjSize_, slab_alloc_page_function_t pfAlloc_, slab_free_page_function_t pfFree_)
{
    m_pfSlabAlloc = pfAlloc_;
    m_pfSlabFree  = pfFree_;
    m_u32ObjSize  = u32ObjSize_;
    m_clFreeList.Init();
    m_clFullList.Init();
}

//---------------------------------------------------------------------------
void* Slab::Alloc(void)
{
    // Allocate from free page list
    auto* pclCurr = reinterpret_cast<SlabPage*>(m_clFreeList.GetHead());
    if (!pclCurr) {
        pclCurr = AllocSlabPage();
        if (!pclCurr) {
            return NULL;
        }
    }
    void* pvRC = pclCurr->Alloc(pclCurr);
    if (pclCurr->IsFull()) {
        MoveToFull(pclCurr);
    }
    return pvRC;
}

//---------------------------------------------------------------------------
void Slab::Free(void* pvObj_)
{
    // Get page from object data
    auto* pstObj_ = reinterpret_cast<bitmap_alloc_t*>((K_ADDR)pvObj_ - (sizeof(bitmap_alloc_t) - sizeof(K_WORD)));
    auto* pclPage = reinterpret_cast<SlabPage*>(pstObj_->pvTag);

    if (pclPage->IsFull()) {
        MoveToFree(pclPage);
    }

    pclPage->Free(pvObj_);

    if (pclPage->IsEmpty()) {
        FreeSlabPage(pclPage);
    }
}

//---------------------------------------------------------------------------
SlabPage* Slab::AllocSlabPage(void)
{
    uint32_t u32PageSize;
    auto*    pclNewPage = reinterpret_cast<SlabPage*>(m_pfSlabAlloc(&u32PageSize));
    if (!pclNewPage) {
        return NULL;
    }

    pclNewPage->InitPage(u32PageSize, m_u32ObjSize);

    m_clFreeList.Add(pclNewPage);
    return pclNewPage;
}

//---------------------------------------------------------------------------
uint32_t Slab::GetFullPageCount()
{
    uint32_t count = 0;
    auto*    node  = m_clFullList.GetHead();
    while (node) {
        count++;
        node = node->GetNext();
    }
    return count;
}

//---------------------------------------------------------------------------
uint32_t Slab::GetFreePageCount()
{
    uint32_t count = 0;
    auto*    node  = m_clFreeList.GetHead();
    while (node) {
        count++;
        node = node->GetNext();
    }
    return count;
}

//---------------------------------------------------------------------------
void Slab::FreeSlabPage(SlabPage* pclPage_)
{
    m_clFreeList.Remove(pclPage_);
    m_pfSlabFree(pclPage_);
}

//---------------------------------------------------------------------------
void Slab::MoveToFull(SlabPage* pclPage_)
{
    m_clFreeList.Remove(pclPage_);
    m_clFullList.Add(pclPage_);
}

//---------------------------------------------------------------------------
void Slab::MoveToFree(SlabPage* pclPage_)
{
    m_clFullList.Remove(pclPage_);
    m_clFreeList.Add(pclPage_);
}
} // namespace Mark3
