//
// Created by phosg on 1/24/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

namespace zp
{
    namespace
    {
        IMemoryAllocator* s_memoryAllocators[static_cast<MemoryLabel>(kMaxMemoryLabels)];
    }

    void RegisterAllocator( const MemoryLabel memoryLabel, IMemoryAllocator* memoryAllocator )
    {
        ZP_ASSERT( memoryLabel < static_cast<MemoryLabel>(kMaxMemoryLabels));
        s_memoryAllocators[ memoryLabel ] = memoryAllocator;
    }

    IMemoryAllocator* GetAllocator( const MemoryLabel memoryLabel )
    {
        ZP_ASSERT( memoryLabel < static_cast<MemoryLabel>(kMaxMemoryLabels));
        return s_memoryAllocators[ memoryLabel ];
    }
}