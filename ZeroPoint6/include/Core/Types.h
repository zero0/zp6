//
// Created by phosg on 11/5/2021.
//

#ifndef ZP_TYPES_H
#define ZP_TYPES_H

typedef bool zp_bool_t;

typedef signed char zp_int8_t;
typedef unsigned char zp_uint8_t;

typedef signed short zp_int16_t;
typedef unsigned short zp_uint16_t;

#define USE_LONG_AS_INT 1
#if USE_LONG_AS_INT
typedef signed long zp_int32_t;
typedef unsigned long zp_uint32_t;
#else
typedef signed int zp_int32_t;
typedef unsigned int zp_uint32_t;
#endif

typedef signed long long zp_int64_t;
typedef unsigned long long zp_uint64_t;

typedef zp_uint16_t zp_float16_t;
typedef float zp_float32_t;
typedef double zp_float64_t;

typedef void* zp_handle_t;
#define ZP_NULL_HANDLE  static_cast<zp_handle_t>(nullptr)

#define ZP_BOOL8( x )       zp_uint8_t x : 1
#define ZP_BOOL16( x )      zp_uint16_t x : 1
#define ZP_BOOL32( x )      zp_uint32_t x : 1
#define ZP_BOOL64( x )      zp_uint64_t x : 1

#ifdef ZP_ARCH64
typedef unsigned long long zp_size_t;
typedef long long zp_ptrdiff_t;
typedef unsigned long long zp_ptr_t;
#else
typedef unsigned int zp_size_t;
typedef signed int zp_ptrdiff_t;
typedef unsigned int zp_ptr_t;
#endif

//
//
//

typedef zp_uint64_t zp_hash64_t;

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
    return !( lh.m32 == rh.m32 && lh.m10 == rh.m10 );
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

typedef zp_uint64_t zp_time_t;

#endif //ZP_TYPES_H
