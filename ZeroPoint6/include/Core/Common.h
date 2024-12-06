//
// Created by phosg on 1/11/2022.
//

#ifndef ZP_COMMON_H
#define ZP_COMMON_H

#include <typeindex>

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"

#if ZP_MSC
#include <intrin.h>
#endif

constexpr zp_size_t ZP_NPOS = -1;

#if ZP_USE_PRINTF

zp_int32_t zp_printf( const char* format, ... );

zp_int32_t zp_printfln( const char* format, ... );

zp_int32_t zp_error_printf( const char* format, ... );

zp_int32_t zp_error_printfln( const char* format, ... );

#else
#define zp_printf(...)     (void)0

#define zp_printfln(...)   (void)0

#define zp_error_printf(...)    (void)0

#define zp_error_printfln(...)  (void)0
#endif

zp_int32_t zp_snprintf( char* dest, zp_size_t destSize, const char* format, ... );

template<zp_size_t Size, typename ... Args>
zp_int32_t zp_snprintf( char (& dest)[Size], const char* format, Args ... args )
{
    return zp_snprintf( static_cast<char*>(dest), Size, format, args... );
}

template<typename ... Args>
zp_int32_t zp_snprintf( zp_char8_t* dest, zp_size_t destSize, const char* format, Args ... args )
{
    return zp_snprintf( reinterpret_cast<char*>(dest), destSize, format, args... );
}

template<zp_size_t Size, typename ... Args>
zp_int32_t zp_snprintf( zp_char8_t (& dest)[Size], const char* format, Args ... args )
{
    return zp_snprintf( reinterpret_cast<char*>(dest), Size, format, args... );
}

#define ZP_STATIC_ASSERT( t )           static_assert( (t), #t )

#if ZP_USE_ASSERTIONS
#define ZP_ASSERT( t )                                      do { if( !(t) ) { zp_error_printfln( "Assertion failed %s:%d - " #t, __FILE__, __LINE__ ); }} while( false )
#define ZP_ASSERT_RETURN( t )                               do { if( !(t) ) { zp_error_printfln( "Assertion failed %s:%d - " #t, __FILE__, __LINE__ ); return; }} while( false )
#define ZP_ASSERT_RETURN_VALUE( t, v )                      do { if( !(t) ) { zp_error_printfln( "Assertion failed %s:%d - " #t, __FILE__, __LINE__ ); return v; }} while( false )
#define ZP_ASSERT_MSG( t, msg )                             do { if( !(t) ) { zp_error_printfln( "Assertion failed %s:%d - " #t ": " msg, __FILE__, __LINE__ ); }} while( false )
#define ZP_ASSERT_MSG_ARGS( t, msg, args... )               do { if( !(t) ) { zp_error_printfln( "Assertion failed %s:%d - " #t ": " msg, __FILE__, __LINE__, args ); }} while( false )
#define ZP_INVALID_CODE_PATH()                              do { zp_error_printfln( "Invalid Code Path %s:%d", __FILE__, __LINE__ ); } while( false )
#define ZP_INVALID_CODE_PATH_MSG( msg )                     do { zp_error_printfln( "Invalid Code Path %s:%d - " msg, __FILE__, __LINE__ ); } while( false )
#define ZP_INVALID_CODE_PATH_MSG_ARGS( msg, args... )       do { zp_error_printfln( "Invalid Code Path %s:%d - " msg, __FILE__, __LINE__, args ); } while( false )
#else // !ZP_USE_ASSERTIONS
#define ZP_ASSERT(...)                      (void)0
#define ZP_ASSERT_RETURN(...)               (void)0
#define ZP_ASSERT_RETURN_VALUE(...)         (void)0
#define ZP_ASSERT_MSG(...)                  (void)0
#define ZP_ASSERT_MSG_ARGS(...)             (void)0
#define ZP_INVALID_CODE_PATH()              (void)0
#define ZP_INVALID_CODE_PATH_MSG(...)       (void)0
#define ZP_INVALID_CODE_PATH_MSG_ARGS(...)  (void)0
#endif // ZP_USE_ASSERTIONS

zp_int32_t zp_atoi32( const char* str, zp_int32_t base = 10 );

zp_int64_t zp_atoi64( const char* str, zp_int32_t base = 10 );

zp_float32_t zp_atof32( const char* str );

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
#define zp_upper_pow2_size( x )   zp_upper_pow2_generic<zp_size_t, 6>( x )
#else
#define zp_upper_pow2_size(x)   zp_upper_pow2_generic<zp_size_t, 5>( x )
#endif

template<typename T, zp_size_t Size>
constexpr zp_size_t zp_array_size( T(&)[Size] )
{
    return Size;
}

constexpr zp_bool_t zp_is_pow2( zp_uint32_t x )
{
    return ( x & ( x - 1 ) ) == 0;
}

constexpr zp_size_t zp_align_size( zp_size_t size, zp_size_t alignment )
{
    return ( size + ( alignment - 1 ) ) & -alignment;
}

void zp_memcpy( void* dst, zp_size_t dstLength, const void* src, zp_size_t srcLength );

void zp_memset( void* dst, zp_size_t dstLength, zp_int32_t value );

template<typename T>
struct zp_remove_reference
{
    typedef T type;
};

template<typename T>
struct zp_remove_pointer
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
    typedef T* type;
};

template<typename T>
struct zp_remove_pointer<T*>
{
    typedef T type;
};

template<typename T>
using zp_remove_reference_t = typename zp_remove_reference<T>::type;

template<typename T>
using zp_remove_pointer_t = typename zp_remove_pointer<T>::type;

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

zp_guid128_t zp_generate_unique_guid128();

zp_guid128_t zp_generate_guid128();

//
//
//

template<typename T>
constexpr zp_int32_t zp_cmp( const T& lh, const T& rh )
{
    return lh < rh ? -1 : rh < lh ? 1 : 0;
}

template<typename T>
constexpr zp_int32_t zp_cmp_asc( const T& lh, const T& rh )
{
    return lh < rh ? -1 : rh < lh ? 1 : 0;
}

template<typename T>
constexpr zp_int32_t zp_cmp_dsc( const T& lh, const T& rh )
{
    return lh < rh ? 1 : rh < lh ? -1 : 0;
}

//
//
//

template<typename T, typename Eq>
constexpr T* zp_find( T* begin, T* end, const T& value, Eq eq )
{
    T* found = nullptr;

    for( ; begin != end; ++begin )
    {
        if( eq( *begin, value ) )
        {
            found = begin;
            break;
        }
    }

    return found;
}

template<typename T>
constexpr T* zp_find( T* begin, T* end, const T& value )
{
    T* found = zp_find( begin, end, value, []( const T& lh, const T& rh ) -> zp_bool_t { return lh == rh; } );
    return found;
}

template<typename T, typename Eq>
constexpr zp_size_t zp_find_index( T* begin, T* end, const T& value, Eq eq )
{
    T* found = zp_find( begin, end, value, eq );
    return found == nullptr ? -1 : found - begin;
}

template<typename T, typename Eq>
constexpr zp_size_t zp_index_of( T* begin, T* end, const T& value )
{
    T* found = zp_find( begin, end, value );
    return found == nullptr ? -1 : found - begin;
}

template<typename T, typename Eq>
constexpr zp_bool_t zp_try_find_index( T* begin, T* end, const T& value, Eq eq, zp_size_t& index )
{
    T* element = zp_find( begin, end, value, eq );
    const zp_bool_t found = element != nullptr;
    if( found )
    {
        index = element - begin;
    }
    return found;
}

template<typename T, typename Eq>
constexpr zp_bool_t zp_try_index_of( T* begin, T* end, const T& value, zp_size_t& index )
{
    T* element = zp_find( begin, end, value );
    const zp_bool_t found = element != nullptr;
    if( found )
    {
        index = element - begin;
    }
    return found;
}

template<typename T, zp_size_t Size, typename Eq>
constexpr zp_size_t zp_find_index( const T (& arr)[Size], const T& value, Eq eq )
{
    zp_size_t index = -1;

    for( zp_size_t i = 0; i < Size; ++i )
    {
        if( eq( arr[ i ], value ) )
        {
            index = i;
            break;
        }
    }

    return index;
}

template<typename T, zp_size_t Size, typename Eq>
constexpr zp_bool_t zp_try_find_index( const T (& arr)[Size], const T& value, Eq eq, zp_size_t& index )
{
    zp_bool_t found = false;

    for( zp_size_t i = 0; i < Size; ++i )
    {
        if( eq( arr[ i ], value ) )
        {
            index = i;
            found = true;
            break;
        }
    }

    return found;
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
            ZP_ASSERT( sizeof( T ) <= size );
            return static_cast<T*>( ptr );
        }

        template<typename T>
        ZP_FORCEINLINE const T* as() const
        {
            ZP_ASSERT( sizeof( T ) <= size );
            return static_cast<const T*>( ptr );
        }

        [[nodiscard]] ZP_FORCEINLINE Memory slice( zp_ptrdiff_t offset, zp_size_t sz ) const
        {
            return {
                .ptr = ZP_OFFSET_PTR( ptr, offset ),
                .size = sz
            };
        }
    };
}

//
//
//

namespace zp
{
    struct SizeInfo
    {
        zp_float32_t size;
        zp_char8_t mem[4];
    };

    constexpr SizeInfo GetSizeInfoFromBytes( zp_size_t bytes, zp_bool_t asBits = false )
    {
        SizeInfo info {};

        if( asBits )
        {
            info.mem[1] = 'b';
            if( bytes > 1 tb )
            {
                info.size = (zp_float32_t)( (zp_float64_t)bytes / ( 1 tb ) );
                info.mem[0] = 't';
            }
            else if( bytes > 1 gb )
            {
                info.size = (zp_float32_t)bytes / (zp_float32_t)( 1 gb );
                info.mem[0] = 'g';
            }
            else if( bytes > 1 mb )
            {
                info.size = (zp_float32_t)bytes / ( 1 mb );
                info.mem[0] = 'm';
            }
            else if( bytes > 1 kb )
            {
                info.size = (zp_float32_t)bytes / ( 1 kb );
                info.mem[0] = 'k';
            }
            else
            {
                info.size = (zp_float32_t)bytes;
                info.mem[0] = 'b';
                info.mem[1] = '\0';
            }
        }
        else
        {
            info.mem[1] = 'B';
            if( bytes > 1 TB )
            {
                info.size = (zp_float32_t)( (zp_float64_t)bytes / ( 1 TB ) );
                info.mem[0] = 'T';
            }
            else if( bytes > 1 GB )
            {
                info.size = (zp_float32_t)bytes / (zp_float32_t)( 1 GB );
                info.mem[0] = 'G';
            }
            else if( bytes > 1 MB )
            {
                info.size = (zp_float32_t)bytes / ( 1 MB );
                info.mem[0] = 'M';
            }
            else if( bytes > 1 KB )
            {
                info.size = (zp_float32_t)bytes / ( 1 KB );
                info.mem[0] = 'K';
            }
            else
            {
                info.size = (zp_float32_t)bytes;
                info.mem[0] = 'B';
                info.mem[1] = '\0';
            }
        }

        return info;
    }
}

#endif //ZP_COMMON_H
