//
// Created by phosg on 7/4/2023.
//

#include "Core/String.h"

#include <cstring>

zp_int32_t zp_strcmp( const char* lh, const char* rh )
{
    return ::strcmp( lh, rh );
}
