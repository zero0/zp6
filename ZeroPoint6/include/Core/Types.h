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

typedef signed int zp_int32_t;
typedef unsigned int zp_uint32_t;

typedef signed long long zp_int64_t;
typedef unsigned long long zp_uint64_t;

typedef void* zp_handle_t;
#define ZP_NULL_HANDLE  static_cast<zp_handle_t>(nullptr)

#define ZP_BOOL8( x )       zp_uint8_t #x : 1
#define ZP_BOOL16( x )      zp_uint16_t #x : 1
#define ZP_BOOL32( x )      zp_uint32_t #x : 1
#define ZP_BOOL64( x )      zp_uint64_t #x : 1

#ifdef ZP_ARCH64
typedef unsigned long long zp_size_t;
typedef long long zp_ptrdiff_t;
#else
typedef unsigned int zp_size_t;
typedef signed int zp_ptrdiff_t;
#endif

typedef zp_uint64_t zp_hash64_t;

typedef struct
{
    zp_uint64_t m01;
    zp_uint64_t m23;
} zp_hash128_t;

#endif //ZP_TYPES_H
