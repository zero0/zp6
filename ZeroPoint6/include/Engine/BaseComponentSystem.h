//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_BASECOMPONENTSYSTEM_H
#define ZP_BASECOMPONENTSYSTEM_H

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Engine/Component.h"
#include "Core/Allocator.h"

namespace zp
{
    class ZP_DECLSPEC_NOVTABLE BaseComponentSystem
    {
    ZP_NONCOPYABLE( BaseComponentSystem )

    public:
        BaseComponentSystem( MemoryLabel memoryLabel );

        virtual ~BaseComponentSystem();

    private:
        ComponentSignature m_componentSignature;
    };
}

#endif //ZP_BASECOMPONENTSYSTEM_H
