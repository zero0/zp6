//
// Created by phosg on 7/4/2023.
//

#include "Core/String.h"

#include <cstring>

constexpr zp_bool_t zp_strempty( const zp_char8_t* str )
{
    return !( str != nullptr && str[ 0 ] != '\0' );
}
