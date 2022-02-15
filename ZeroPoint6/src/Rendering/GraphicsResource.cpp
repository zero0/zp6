//
// Created by phosg on 2/12/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Atomic.h"

#include "Rendering/GraphicsResource.h"

namespace zp
{
    void BaseGraphicsResource::addRef()
    {
        Atomic::IncrementSizeT( &m_refCount );
    }

    void BaseGraphicsResource::removeRef()
    {
        Atomic::DecrementSizeT( &m_refCount );
    }
}