//
// Created by phosg on 9/20/2024.
//

#ifndef ZP_HASH_H
#define ZP_HASH_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Memory.h"

//
//
//

constexpr zp_hash64_t zp_fnv64_1a( const void* ptr, const zp_size_t size, zp_hash64_t h = 0xcbf29ce484222325 )
{
    if( ptr )
    {
        auto data = static_cast<const zp_uint8_t*>( ptr );

        const zp_hash64_t prime { 0x00000100000001b3 };
        for( zp_size_t i = 0; i != size; ++i )
        {
            h ^= static_cast<zp_uint64_t>(data[ i ]);
            h *= prime;
        }
    }

    return h;
}

template<typename T>
constexpr zp_hash64_t zp_fnv64_1a( const T& value, zp_hash64_t h = 0xcbf29ce484222325 )
{
    return zp_fnv64_1a( static_cast<const void*>(&value), sizeof( T ), h );
}

template<typename T>
constexpr zp_hash64_t zp_fnv64_1a( const T* value, zp_size_t length, zp_hash64_t h = 0xcbf29ce484222325 )
{
    return zp_fnv64_1a( static_cast<const void*>(value), sizeof( T ) * length, h );
}

template<typename T, zp_size_t Size>
constexpr zp_hash64_t zp_fnv64_1a( const T (& value)[Size], zp_hash64_t h = 0xcbf29ce484222325 )
{
    return zp_fnv64_1a( static_cast<const void*>(&value), sizeof( T ) * Size, h );
}

constexpr zp_hash64_t zp_fnv64_1a( const zp::Memory& memory, zp_hash64_t h = 0xcbf29ce484222325 )
{
    return zp_fnv64_1a( memory.ptr, memory.size, h );
}

//
//
//

constexpr zp_hash128_t zp_fnv128_1a( const void* ptr, const zp_size_t size, zp_hash128_t h = { .m32 = 0x6c62272e07bb0142, .m10 = 0x62b821756295c58d } )
{
    if( ptr )
    {
        auto data = static_cast<const zp_uint8_t*>( ptr );

        zp_uint64_t a, b, c, d;
        a = (zp_uint32_t)( h.m32 >> 32 );
        b = (zp_uint32_t)( h.m32 );
        c = (zp_uint32_t)( h.m10 >> 32 );
        d = (zp_uint32_t)( h.m10 );

        const zp_uint32_t P128_B = 0x01000000, P128_D = 0x0000013B, P128_BD = 0xFFFEC5;

        zp_uint64_t f, fLm;

        for( zp_size_t i = 0; i != size; ++i )
        {
            d ^= data[ i ];

            {
                // Code from: https://github.com/3F/Fnv1a128/blob/master/src/csharp/Fnv1a.cs
                // Below is an optimized implementation (limited) of the LX4Cnh algorithm specially for Fnv1a128
                // (c) Denis Kuzmin <x-3F@outlook.com> github/3F

                f = b * P128_B;

                zp_uint64_t v = (zp_uint32_t)f;

                f = ( f >> 32 ) + v;

                if( a > b )
                {
                    f += (zp_uint32_t)( ( a - b ) * P128_B );
                }
                else if( a < b )
                {
                    f -= (zp_uint32_t)( ( b - a ) * P128_B );
                }

                zp_uint64_t fHigh = ( f << 32 ) + (zp_uint32_t)v;
                zp_uint64_t r2 = d * P128_D;

                v = ( r2 >> 32 ) + ( r2 & 0xFFFFFFFFFFFFFFF );

                f = ( r2 & 0xF000000000000000 ) >> 32;

                if( c > d )
                {
                    fLm = v;
                    v += ( c - d ) * P128_D;
                    if( fLm > v )
                    { f += 0x100000000; }
                }
                else if( c < d )
                {
                    fLm = v;
                    v -= ( d - c ) * P128_D;
                    if( fLm < v )
                    { f -= 0x100000000; }
                }

                fLm = ( ( (zp_uint64_t)(zp_uint32_t)v ) << 32 ) + (zp_uint32_t)r2;

                f = fHigh + fLm + f + ( v >> 32 );

                fHigh = ( a << 32 ) + b; //fa
                v = ( c << 32 ) + d; //fb

                if( fHigh < v )
                {
                    f += ( v - fHigh ) * P128_BD;
                }
                else if( fHigh > v )
                {
                    f -= ( fHigh - v ) * P128_BD;
                }
            }

            a = (zp_uint32_t)( f >> 32 );
            b = (zp_uint32_t)f;
            c = (zp_uint32_t)( fLm >> 32 );
            d = (zp_uint32_t)fLm;
        }

        h.m32 = ( a << 32 ) | b;
        h.m10 = ( c << 32 ) | d;
    }

    return h;
}

template<typename T>
constexpr zp_hash128_t zp_fnv128_1a( const T* value, zp_size_t count, zp_hash128_t h = { .m32 = 0x6c62272e07bb0142, .m10 = 0x62b821756295c58d } )
{
    return zp_fnv128_1a( static_cast<const void*>(value), sizeof( T ) * count, h );
}

template<typename T>
constexpr zp_hash128_t zp_fnv128_1a( const T& value, zp_hash128_t h = { .m32 = 0x6c62272e07bb0142, .m10 = 0x62b821756295c58d } )
{
    return zp_fnv128_1a( static_cast<const void*>(&value), sizeof( T ), h );
}

template<typename T, zp_size_t Count>
constexpr zp_hash128_t zp_fnv128_1a( const T( & array )[Count], zp_hash128_t h = { .m32 = 0x6c62272e07bb0142, .m10 = 0x62b821756295c58d } )
{
    return zp_fnv128_1a( static_cast<const void*>(array), sizeof( T ) * Count, h );
}

constexpr zp_hash128_t zp_fnv128_1a( const zp::Memory& memory, zp_hash128_t h = { .m32 = 0x6c62272e07bb0142, .m10 = 0x62b821756295c58d } )
{
    return zp_fnv128_1a( memory.ptr, memory.size, h );
}

//
//
//

template<typename H>
H zp_fnv_1a( const void* ptr, zp_size_t size );

template<>
constexpr zp_hash64_t zp_fnv_1a<zp_hash64_t>( const void* ptr, zp_size_t size )
{
    return zp_fnv64_1a( ptr, size );
}

template<>
constexpr zp_hash128_t zp_fnv_1a<zp_hash128_t>( const void* ptr, zp_size_t size )
{
    return zp_fnv128_1a( ptr, size );
}

//
//
//

namespace zp
{
    template<typename T>
    struct OperatorComparer
    {
        typedef const T& const_reference;

        zp_int32_t operator()( const_reference lh, const_reference rh )
        {
            const zp_int32_t cmp = lh < rh ? -1 : rh < lh ? 1 : 0;
            return cmp;
        }
    };

    template<typename T>
    struct StaticFunctionComparer
    {
        typedef const T& const_reference;

        zp_int32_t operator()( const_reference lh, const_reference rh )
        {
            const zp_int32_t cmp = T::compare( lh, rh );
            return cmp;
        }
    };

    //
    //
    //

    template<typename T, typename H>
    struct DefaultHash
    {
        typedef const T& const_reference;

        H operator()( const_reference val ) const
        {
            return zp_fnv_1a<H>( &val, sizeof( T ) );
        }
    };

    template<typename T>
    struct DefaultEquality
    {
        typedef const T& const_reference;

        zp_bool_t operator()( const_reference lh, const_reference rh ) const
        {
            return lh == rh;
        }
    };

    template<typename T, typename H = zp_hash64_t>
    struct EqualityComparer
    {
        typedef const T& const_reference;

        H hash( const_reference val ) const
        {
            return zp_fnv_1a<H>( &val, sizeof( T ) );
        }

        zp_bool_t equals( const_reference lh, const_reference rh ) const
        {
            return lh == rh;
        }
    };

    template<typename T, typename H = zp_hash64_t>
    struct CastEqualityComparer
    {
        typedef const T& const_reference;

        H hash( const_reference val ) const
        {
            return static_cast<H>( val );
        }

        zp_bool_t equals( const_reference lh, const_reference rh ) const
        {
            return lh == rh;
        }
    };

    template<typename T, typename H = zp_hash64_t>
    struct OperatorEqualityComparer
    {
        typedef const T& const_reference;

        H hash( const_reference val ) const
        {
            return T::operator H( val );
        }

        zp_bool_t equals( const_reference lh, const_reference rh ) const
        {
            return lh == rh;
        }
    };

    template<typename H>
    struct HashEqualityComparer
    {
        typedef const H& const_reference;

        H hash( const_reference val ) const
        {
            return val;
        }

        zp_bool_t equals( const_reference lh, const_reference rh ) const
        {
            return lh == rh;
        }
    };

    typedef HashEqualityComparer<zp_hash64_t> Hash64EqualityComparer;

    typedef HashEqualityComparer<zp_hash128_t> Hash128EqualityComparer;
}

#endif //ZP_HASH_H
