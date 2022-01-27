//
// Created by phosg on 1/11/2022.
//

#ifndef ZP_COMMON_H
#define ZP_COMMON_H

#include <typeindex>

#include "Core/Defines.h"
#include "Core/Types.h"

#if ZP_MSC
#include <intrin.h>
#endif

#if ZP_USE_PRINTF

void zp_printf( const char* format, ... );

void zp_printfln( const char* format, ... );

#else
#define zp_printf(...)     (void)0

#define zp_printfln(...)   (void)0
#endif

#define ZP_STATIC_ASSERT( t )   static_assert( (t), #t )

#if ZP_USE_ASSERTIONS
#define ZP_ASSERT( t )            do { if( !(t) ) { zp_printfln( "Assertion failed %s:%d - %s", __FILE__, __LINE__, #t ); }} while( false )
#define ZP_ASSERT_MSG( t, msg )   do { if( !(t) ) { zp_printfln( "Assertion failed %s:%d - %s", __FILE__, __LINE__, msg ); }} while( false )
#else // !ZP_USE_ASSERTIONS
#define ZP_ASSERT(...)      (void)0
#define ZP_ASSERT_MSG(...)  (void)0
#endif // ZP_USE_ASSERTIONS

template<typename T, int CountLog2>
constexpr T zp_upper_pow2_generic( T val )
{
    --val;
    for( int i = 0; i < CountLog2; ++i )
    {
        val |= val >> (1 << i);
    }
    ++val;
    return val;
}

#define zp_upper_pow2_8( x )      zp_upper_pow2_generic<zp_uint8_t, 3>( x )
#define zp_upper_pow2_16( x )     zp_upper_pow2_generic<zp_uint16_t, 4>( x )
#define zp_upper_pow2_32( x )     zp_upper_pow2_generic<zp_uint32_t, 5>( x )
#define zp_upper_pow2_64( x )     zp_upper_pow2_generic<zp_uint64_t, 6>( x )

#if ZP_ARCH64
#define zp_upper_pow2_size( x )   zp_upper_pow2_generic<zp_size_t, 5>( x )
#else
#define zp_upper_pow2_size(x)   zp_upper_pow2_generic<zp_size_t, 6>( x )
#endif

void zp_memcpy( void* dst, zp_size_t dstLength, const void* src, zp_size_t srcLength );

void zp_memset( void* dst, zp_size_t dstLength, zp_int32_t value );

template<typename T>
struct zp_remove_reference
{
    typedef T type;
};

template<typename T>
struct zp_remove_reference<T&>
{
    typedef T type;
};

template<typename T>
struct zp_remove_reference<T&&>
{
    typedef T type;
};

template<typename T>
using zp_remove_reference_t = typename zp_remove_reference<T>::type;

template<typename T>
constexpr T&& zp_forward( zp_remove_reference_t<T>& val )
{
    return static_cast<T&&>( val );
}

template<typename T>
constexpr T&& zp_forward( zp_remove_reference_t<T>&& val )
{
    return static_cast<T&&>( val );
}

template<typename T>
constexpr zp_remove_reference_t<T>&& zp_move( T&& val )
{
    return static_cast<zp_remove_reference_t<T>&&>( val );
}

ZP_FORCEINLINE void zp_zero_memory( void* ptr, zp_size_t size )
{
    zp_memset( ptr, size, 0 );
}

template<typename T>
ZP_FORCEINLINE void zp_zero_memory( T* ptr )
{
    zp_memset( ptr, sizeof( T ), 0 );
}

template<typename T>
ZP_FORCEINLINE void zp_zero_memory_array( T* arr, zp_size_t length )
{
    zp_memset( arr, sizeof( T ) * length, 0 );
}

template<typename T>
constexpr zp_hash64_t zp_type_hash()
{
    return typeid( T ).hash_code();
}

constexpr zp_int32_t zp_bitscan_forward( zp_uint64_t val )
{
#if ZP_GNUC
    return __builtin_ctzll( val );
#elif ZP_MSC
    unsigned long index;
    _BitScanForward64( &index, val );
    return index;
#else
#error "Unknown Compiler type"
#endif
}

constexpr zp_int32_t zp_bitscan_reverse( zp_uint64_t val )
{
#if ZP_GNUC
    return __builtin_clzll( val ) ^ 63;
#elif ZP_MSC
    unsigned long index;
    _BitScanReverse64( &index, val );
    return index;
#else
#error "Unknown Compiler type"
#endif
}

#endif //ZP_COMMON_H
