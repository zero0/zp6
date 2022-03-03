//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_COMPONENTSIGNATURE_H
#define ZP_COMPONENTSIGNATURE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"

namespace zp
{
    typedef zp_uint32_t TagType;
    typedef zp_uint32_t ComponentType;

    typedef zp_uint64_t TagSignature;
    typedef zp_uint64_t StructuralSignature;

    struct ComponentSignature
    {
        TagSignature tagSignature;
        StructuralSignature structuralSignature;

        ComponentSignature& addComponent( ComponentType type )
        {
            structuralSignature |= 1 << type;
            return *this;
        }

        ComponentSignature& removeComponent( ComponentType type )
        {
            structuralSignature &= ~( 1 << type );
            return *this;
        }

        ComponentSignature& addTag( TagType type )
        {
            tagSignature |= 1 << type;
            return *this;
        }

        ComponentSignature& removeTag( TagType type )
        {
            tagSignature &= ~( 1 << type );
            return *this;
        }
    };
}

#endif //ZP_COMPONENTSIGNATURE_H
