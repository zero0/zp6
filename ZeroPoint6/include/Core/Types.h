//
// Created by phosg on 11/5/2021.
//

#ifndef ZP_TYPES_H
#define ZP_TYPES_H

#include "Core/Defines.h"

using zp_bool_t = bool;

using zp_int8_t = signed char;
using zp_uint8_t = unsigned char;

using zp_int16_t = short;
using zp_uint16_t = unsigned short;

#define USE_LONG_AS_INT 0
#if USE_LONG_AS_INT
using zp_int32_t = long;
using zp_uint32_t = unsigned long;
#else
using zp_int32_t = int;
using zp_uint32_t = unsigned int;
#endif

using zp_int64_t = long long;
using zp_uint64_t = unsigned long long;

using zp_float16_t = zp_uint16_t;
using zp_float32_t = float;
using zp_float64_t = double;

using zp_handle_t = void *;
constexpr zp_handle_t ZP_NULL_HANDLE = static_cast<zp_handle_t>(nullptr);

#define ZP_BOOL8( x )       zp_uint8_t x : 1
#define ZP_BOOL16( x )      zp_uint16_t x : 1
#define ZP_BOOL32( x )      zp_uint32_t x : 1
#define ZP_BOOL64( x )      zp_uint64_t x : 1

#ifdef ZP_PLATFORM_ARCH64
using zp_size_t = unsigned long long;
using zp_ptrdiff_t = long long;
using zp_ptr_t = unsigned long long;
#else
using zp_size_t = unsigned int;
using zp_ptrdiff_t = int;
using zp_ptr_t = unsigned int;
#endif

#if ZP_USE_UTF8_LITERALS
using zp_char8_t = char8_t;
#else
using zp_char8_t = char;
#endif

using zp_nullptr_t = decltype(nullptr);

//
//
//

using zp_hash32_t = zp_uint32_t;

using zp_hash64_t = zp_uint64_t;

struct zp_hash128_t
{
    zp_uint64_t m32;
    zp_uint64_t m10;
};

ZP_FORCEINLINE zp_bool_t operator==( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return lh.m32 == rh.m32 && lh.m10 == rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator!=( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return !( lh == rh );
}

ZP_FORCEINLINE zp_bool_t operator<( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 < rh.m32 : lh.m10 < rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator>( const zp_hash128_t& lh, const zp_hash128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 > rh.m32 : lh.m10 > rh.m10;
}

ZP_FORCEINLINE zp_size_t operator%( const zp_hash128_t& lh, zp_size_t rh )
{
    return ( lh.m32 ^ lh.m10 ) % rh;
}

//
//
//

struct zp_guid128_t
{
    zp_uint64_t m32;
    zp_uint64_t m10;

    constexpr explicit operator zp_hash128_t() const
    {
        return { .m32 = m32, .m10 = m10 };
    }
};

ZP_FORCEINLINE zp_bool_t operator==( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return lh.m32 == rh.m32 && lh.m10 == rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator!=( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return !( lh.m32 == rh.m32 && lh.m10 == rh.m10 );
}

ZP_FORCEINLINE zp_bool_t operator<( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 < rh.m32 : lh.m10 < rh.m10;
}

ZP_FORCEINLINE zp_bool_t operator>( const zp_guid128_t& lh, const zp_guid128_t& rh )
{
    return lh.m32 != rh.m32 ? lh.m32 > rh.m32 : lh.m10 > rh.m10;
}

//
//
//

using zp_time_t = zp_uint64_t;

#define ZP_TIME_INFINITE    static_cast<zp_time_t>( ~0 )

//
//
//

namespace zp
{
    using MemoryLabel = zp_uint8_t;
}

//
//
//

namespace zp
{
    enum
    {
        kMaxStackTraceDepth = 30,
    };

    struct StackTrace
    {
        //FixedArray<void*, kMaxStackTraceDepth> stack;
        void* stack[kMaxStackTraceDepth];
        zp_size_t length;
        zp_hash64_t hash;
    };
}

#endif //ZP_TYPES_H
