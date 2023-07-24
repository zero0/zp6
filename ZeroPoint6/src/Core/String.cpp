//
// Created by phosg on 7/4/2023.
//

#include "Core/String.h"

#include <cstring>

zp_int32_t zp_strcmp( const char* lh, const char* rh )
{
    return lh && rh ? ::strcmp( lh, rh ) : lh ? -1 : rh ? 1 : 0;
}

zp_int32_t zp_strcmp( const char* lh, zp_size_t lhSize, const char* rh, zp_size_t rhSize )
{
    zp_int32_t cmp = zp_cmp( lhSize, rhSize );

    if( cmp == 0 )
    {
        for( zp_size_t i = 0; i < lhSize && cmp == 0; ++i )
        {
            cmp = zp_cmp( lh[ i ], rh[ i ] );
        }
    }

    return cmp;
}

zp_int32_t zp_strcmp( const zp_char8_t* lh, zp_size_t lhSize, const zp_char8_t* rh, zp_size_t rhSize )
{
    zp_int32_t cmp = zp_cmp( lhSize, rhSize );

    if( cmp == 0 )
    {
        for( zp_size_t i = 0; i < lhSize && cmp == 0; ++i )
        {
            cmp = zp_cmp( lh[ i ], rh[ i ] );
        }
    }

    return cmp;
}
