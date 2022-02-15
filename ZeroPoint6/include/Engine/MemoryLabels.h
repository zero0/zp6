//
// Created by phosg on 1/26/2022.
//

#ifndef ZP_MEMORYLABELS_H
#define ZP_MEMORYLABELS_H

#include "Core/Allocator.h"

namespace zp
{
    enum MemoryLabels : MemoryLabel
    {
        Default,
        String,
        Graphics,
        FileIO,
        Buffer,
        User,
        Data,
        Temp,
        ThreadSafe,

        Profiling,
        Debug,

        MemoryLabels_Count
    };

    static_assert( static_cast<MemoryLabel>(MemoryLabels::MemoryLabels_Count) < kMaxMemoryLabels );
}
#endif //ZP_MEMORYLABELS_H
