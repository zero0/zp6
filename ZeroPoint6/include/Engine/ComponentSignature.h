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
    typedef zp_uint8_t TagType;
    typedef zp_uint8_t ComponentType;

    //typedef GenericComponentSignature<64> ComponentSignature;
    typedef struct {
        zp_uint64_t tagSignature;
        zp_uint64_t structuralSignature;
    } ComponentSignature;
}

#endif //ZP_COMPONENTSIGNATURE_H
