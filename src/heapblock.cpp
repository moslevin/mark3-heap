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
/**

    @file   heapblock.cpp

    @brief  Metadata object used to manage a heap allocation
*/

#include "heapblock.h"
namespace Mark3
{
//---------------------------------------------------------------------------
void HeapBlock::RootInit(K_ADDR usize_)
{
    Init();
    m_uDataSize = ROUND_DOWN(usize_);

    SetCookie(HEAP_COOKIE_FREE);

    SetRightSibling(0);
    SetLeftSibling(0);
}

//---------------------------------------------------------------------------
HeapBlock* HeapBlock::Split(K_ADDR usize_)
{
    // Allocate minimum amount of data for this operation on the left side

    K_ADDR u32eftBlockSize = ROUND_UP(usize_) + sizeof(HeapBlock);

    K_ADDR uThisBlockSize  = m_uDataSize + sizeof(HeapBlock);
    K_ADDR uRightBlockSize = uThisBlockSize - u32eftBlockSize;

    m_uDataSize = ROUND_UP(usize_);

    auto  uNewAddr      = (K_ADDR)this + u32eftBlockSize;
    auto* pclRightBlock = reinterpret_cast<HeapBlock*>(uNewAddr);

    pclRightBlock->Init();
    pclRightBlock->SetDataSize(uRightBlockSize - sizeof(HeapBlock));
    pclRightBlock->SetCookie(HEAP_COOKIE_FREE);

    pclRightBlock->SetLeftSibling(this);
    pclRightBlock->SetRightSibling(GetRightSibling());

    if (GetRightSibling() != 0) {
        GetRightSibling()->SetLeftSibling(pclRightBlock);
    }

    SetRightSibling(pclRightBlock);

    return pclRightBlock;
}

//---------------------------------------------------------------------------
// Merge this block with RIGHT neighbor.
void HeapBlock::Coalesce(void)
{
    auto* pclTemp = GetRightSibling();
    // Add the size of this object to the left object.
    SetDataSize(GetDataSize() + pclTemp->GetBlockSize());

    // Reconnect sibling pointers between this block and "absorbed" right block
    SetRightSibling(pclTemp->GetRightSibling());
    if (GetRightSibling() != 0) {
        GetRightSibling()->SetLeftSibling(this);
    }
}

//---------------------------------------------------------------------------
void* HeapBlock::GetDataPointer(void)
{
    auto uAddr = reinterpret_cast<K_ADDR>(this);
    uAddr += sizeof(HeapBlock);
    return reinterpret_cast<void*>(uAddr);
}

//---------------------------------------------------------------------------
K_ADDR HeapBlock::GetDataSize(void)
{
    return m_uDataSize;
}

//---------------------------------------------------------------------------
K_ADDR HeapBlock::GetBlockSize(void)
{
    return (sizeof(HeapBlock) + m_uDataSize);
}

//---------------------------------------------------------------------------
void HeapBlock::SetArenaIndex(uint8_t u8List_)
{
    m_u8ArenaIndex = u8List_;
}

//---------------------------------------------------------------------------
uint8_t HeapBlock::GetArenaIndex(void)
{
    return m_u8ArenaIndex;
}

//---------------------------------------------------------------------------
void HeapBlock::SetDataSize(K_ADDR uBlockSize)
{
    m_uDataSize = uBlockSize;
}
} // namespace Mark3
