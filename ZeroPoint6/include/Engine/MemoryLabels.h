//
// Created by phosg on 1/26/2022.
//

#ifndef ZP_MEMORYLABELS_H
#define ZP_MEMORYLABELS_H

#include "Core/Allocator.h"

namespace zp
{
    namespace MemoryLabels
    {
        constexpr MemoryLabel Default = 0;
        constexpr MemoryLabel String = 1;
        constexpr MemoryLabel Graphics = 2;
        constexpr MemoryLabel FileIO = 3;
        constexpr MemoryLabel Buffer = 4;
        constexpr MemoryLabel User = 5;
        constexpr MemoryLabel Data = 6;
        constexpr MemoryLabel Temp = 7;
        constexpr MemoryLabel ThreadSafe = 8;

        constexpr MemoryLabel Profiling = 9;
        constexpr MemoryLabel Debug = 10;

        constexpr MemoryLabel MemoryLabels_Count = 11;
    };

    ZP_STATIC_ASSERT( static_cast<MemoryLabel>( MemoryLabels::MemoryLabels_Count ) < kMaxMemoryLabels );
}

#endif //ZP_MEMORYLABELS_H
