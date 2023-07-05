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

zp_int32_t zp_printf( const char* format, ... );

zp_int32_t zp_printfln( const char* format, ... );

#else
#define zp_printf(...)     (void)0

#define zp_printfln(...)   (void)0
#endif

zp_int32_t zp_snprintf( char* dest, zp_size_t destSize, const char* format, ... );

template<zp_size_t Size, typename ... Args>
zp_int32_t zp_snprintf( char (& dest)[Size], const char* format, Args ... args )
{
    return zp_snprintf( static_cast<char*>(dest), Size, format, args... );
}

#define ZP_STATIC_ASSERT( t )           static_assert( (t), #t )

#if ZP_USE_ASSERTIONS
#define ZP_ASSERT( t )                  do { if( !(t) ) { zp_printfln( "Assertion failed %s:%d - %s", __FILE__, __LINE__, #t ); }} while( false )
#define ZP_ASSERT_MSG( t, msg )         do { if( !(t) ) { zp_printfln( "Assertion failed %s:%d - %s: %s", __FILE__, __LINE__, #t, msg ); }} while( false )
#define ZP_INVALID_CODE_PATH()          do { zp_printfln( "Invalid Code Path %s:%d", __FILE__, __LINE__ ); } while( false )
#define ZP_INVALID_CODE_PATH_MSG( msg ) do { zp_printfln( "Invalid Code Path %s:%d - %s", __FILE__, __LINE__, msg ); } while( false )
#else // !ZP_USE_ASSERTIONS
#define ZP_ASSERT(...)                  (void)0
#define ZP_ASSERT_MSG(...)              (void)0
#define ZP_INVALID_CODE_PATH()          (void)0
#define ZP_INVALID_CODE_PATH_MSG(...)   (void)0
#endif // ZP_USE_ASSERTIONS

template<typename T, int CountLog2>
constexpr T zp_upper_pow2_generic( T val )
{
    --val;
    for( int i = 0; i < CountLog2; ++i )
    {
        val |= val >> ( 1 << i );
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

constexpr zp_bool_t zp_is_pow2( zp_uint32_t x )
{
    return ( x & ( x - 1 ) ) == 0;
}

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
struct zp_remove_reference<T*>
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

template<typename T>
constexpr void zp_swap( T& a, T& b )
{
    T t = a;
    a = b;
    b = t;
}

template<typename T>
constexpr void zp_move_swap( T& a, T& b )
{
    T t = zp_move( a );
    a = zp_move( b );
    b = zp_move( t );
}

template<typename T>
constexpr void zp_move_swap( T&& a, T&& b )
{
    T t = zp_move( a );
    a = zp_move( b );
    b = zp_move( zp_forward( t ) );
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

template<typename T, zp_size_t Size>
ZP_FORCEINLINE void zp_zero_memory_array( T( & arr )[Size] )
{
    zp_memset( arr, sizeof( T ) * Size, 0 );
}

template<typename T>
constexpr zp_hash64_t zp_type_hash()
{
    const zp_hash64_t hash { typeid( T ).hash_code() };
    return hash;
}

template<typename T>
constexpr const char* zp_type_name()
{
    return typeid( T ).name();
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

zp_time_t zp_time_now();

zp_time_t zp_time_frequency();

zp_uint64_t zp_time_cycle();

zp_uint32_t zp_current_thread_id();

void zp_yield_current_thread();

void zp_sleep_current_thread( zp_uint64_t timeMilliseconds );

void zp_debug_break();

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

zp_guid128_t zp_generate_unique_guid128();

zp_guid128_t zp_generate_guid128();

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

template<typename T>
constexpr zp_int32_t zp_cmp( const T& lh, const T& rh )
{
    return lh < rh ? -1 : rh < lh ? 1 : 0;
}

//
//
//

zp_size_t zp_lzf_compress( const void* srcBuffer, zp_size_t srcPosition, zp_size_t srcSize, void* dstBuffer, zp_size_t dstPosition );

void zp_lzf_expand( const void* srcBuffer, zp_size_t srcPosition, zp_size_t srcSize, void* dstBuffer, zp_size_t dstPosition, zp_size_t dstSize );

//
//
//

template<typename T, typename Cmp>
constexpr void zp_qsort3( T* begin, T* end, Cmp cmp )
{
    // partition
    T* mid = begin;
    T* pivot = end;

    while( mid <= end )
    {
        const zp_int32_t c = cmp( *mid, *pivot );
        if( c < 0 )
        {
            zp_move_swap( *begin, *mid );
            ++begin;
            ++mid;
        }
        else if( c > 0 )
        {
            zp_move_swap( *mid, *end );
            --end;
        }
        else
        {
            ++mid;
        }
    }

    T* i = begin - 1;
    T* j = mid;

    if( begin < i )
    {
        zp_qsort3( begin, i, cmp );
    }

    if( j < end )
    {
        zp_qsort3( j, end, cmp );
    }
};

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

//
//
//

namespace zp
{
    struct Memory
    {
        void* ptr;
        zp_size_t size;

        template<typename T>
        ZP_FORCEINLINE T* as()
        {
            return static_cast<T*>( ptr );
        }

        template<typename T>
        ZP_FORCEINLINE const T* as() const
        {
            return static_cast<const T*>( ptr );
        }

        [[nodiscard]] ZP_FORCEINLINE Memory slice( zp_ptrdiff_t offset, zp_size_t sz ) const
        {
            return {
                .ptr = static_cast<zp_uint8_t*>( ptr ) + offset,
                .size = sz
            };
        }
    };
}

#endif //ZP_COMMON_H
