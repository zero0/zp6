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
        const MemoryLabel Default = 0;
        const MemoryLabel String = 1;
        const MemoryLabel Graphics = 2;
        const MemoryLabel FileIO = 3;
        const MemoryLabel Buffer = 4;
        const MemoryLabel User = 5;
        const MemoryLabel Data = 6;
        const MemoryLabel Temp = 7;
        const MemoryLabel ThreadSafe = 8;

        const MemoryLabel Profiling = 9;
        const MemoryLabel Debug = 10;

        const MemoryLabel MemoryLabels_Count = 11;
    };

    static_assert( MemoryLabels::MemoryLabels_Count < kMaxMemoryLabels );
}
#endif //ZP_MEMORYLABELS_H
