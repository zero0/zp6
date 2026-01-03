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

namespace zp
{
    constexpr zp_size_t npos = -1;
}


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
    return static_cast<T&&>(val);
}

template<typename T>
constexpr T&& zp_forward( zp_remove_reference_t<T>&& val )
{
    return static_cast<T&&>(val);
}

template<typename T>
constexpr zp_remove_reference_t<T>&& zp_move( T&& val )
{
    return static_cast<zp_remove_reference_t<T>&&>(val);
}


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

template<zp_size_t Size, typename... Args>
zp_int32_t zp_snprintf( char ( &dest )[ Size ], const char* format, Args... args )
{
    return zp_snprintf( static_cast<char*>(dest), Size, format, args... );
}

template<typename... Args>
zp_int32_t zp_snprintf( zp_char8_t* dest, zp_size_t destSize, const char* format, Args... args )
{
    return zp_snprintf( reinterpret_cast<char*>(dest), destSize, format, args... );
}

template<zp_size_t Size, typename... Args>
zp_int32_t zp_snprintf( zp_char8_t ( &dest )[ Size ], const char* format, Args... args )
{
    return zp_snprintf( reinterpret_cast<char*>(dest), Size, format, args... );
}


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

#if ZP_PLATFORM_ARCH64
#define zp_upper_pow2_size( x )   zp_upper_pow2_generic<zp_size_t, 6>( x )
#else
#define zp_upper_pow2_size(x)   zp_upper_pow2_generic<zp_size_t, 5>( x )
#endif

template<typename T, zp_size_t Size>
constexpr zp_size_t zp_array_size( T ( & )[ Size ] )
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

constexpr void* zp_offset_ptr( void* ptr, zp_ptrdiff_t offset )
{
    return static_cast<void*>(static_cast<zp_uint8_t*>(ptr) + offset);
}

constexpr const void* zp_offset_ptr( const void* ptr, zp_ptrdiff_t offset )
{
    return static_cast<const void*>(static_cast<const zp_uint8_t*>(ptr) + offset);
}

void zp_memcpy( void* dst, zp_size_t dstLength, const void* src, zp_size_t srcLength );

void zp_memset( void* dst, zp_size_t dstLength, zp_int32_t value );

zp_int32_t zp_memcmp( const void* lh, zp_size_t lhLength, const void* rh, zp_size_t rhLength );

template<typename TLIter, typename TRIter>
zp_int32_t zp_memcmp( const TLIter* lhBegin, const TLIter* lhEnd, const TRIter* rhBegin, const TRIter* rhEnd )
{
    return zp_memcmp( lhBegin, lhEnd - lhBegin, rhBegin, rhEnd - rhBegin );
}

template<typename TL, typename TR>
zp_int32_t zp_memcmp( const TL& lh, const TR& rh )
{
    return zp_memcmp( lh.begin(), lh.end(), rh.begin(), rh.end() );
}

template<typename T>
zp_int32_t zp_memcmp( const T& lh, const T& rh )
{
    return zp_memcmp( &lh, sizeof( T ), &rh, sizeof( T ) );
}

#define ZP_STATIC_ASSERT( t )           static_assert( (t), #t )

#if ZP_USE_ASSERTIONS

template<typename... Args>
constexpr void zp_assert( const char* msg, const char* file, zp_size_t line, Args... args )
{
    char assertMsg[ 512 ];
    zp_snprintf( assertMsg, msg, args... );

    zp_error_printfln( "Assertion failed %s:%d - %s", file, line, assertMsg );
}

#define ZP_ASSERT( t )                                      do { if( !(t) ) [[unlikely]] { zp_assert( #t, __FILE__, __LINE__ ); }} while( false )
#define ZP_ASSERT_RETURN( t )                               do { if( !(t) ) [[unlikely]] { zp_assert( #t, __FILE__, __LINE__ ); return; }} while( false )
#define ZP_ASSERT_RETURN_VALUE( t, v )                      do { if( !(t) ) [[unlikely]] { zp_assert( #t, __FILE__, __LINE__ ); return v; }} while( false )
#define ZP_ASSERT_MSG( t, msg )                             do { if( !(t) ) [[unlikely]] { zp_assert( #t ": " msg, __FILE__, __LINE__ ); }} while( false )
#define ZP_ASSERT_MSG_ARGS( t, msg, args... )               do { if( !(t) ) [[unlikely]] { zp_assert( #t ": " msg, __FILE__, __LINE__, args ); }} while( false )
#define ZP_INVALID_CODE_PATH()                              do { zp_assert( "Invalid Code Path", __FILE__, __LINE__ ); } while( false )
#define ZP_INVALID_CODE_PATH_MSG( msg )                     do { zp_assert( "Invalid Code Path: " msg, __FILE__, __LINE__ ); } while( false )
#define ZP_INVALID_CODE_PATH_MSG_ARGS( msg, args... )       do { zp_assert( "Invalid Code Path: " msg, __FILE__, __LINE__, args ); } while( false )
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
ZP_FORCEINLINE void zp_zero_memory_array( T ( &arr )[ Size ] )
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

//
//
//

zp_guid128_t zp_generate_unique_guid128();

zp_guid128_t zp_generate_guid128();

zp_guid128_t zp_generate_guid128( const zp_uint8_t (&bytes)[16] );

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

ZP_FORCEINLINE constexpr auto zp_flag32_is_bit_set( zp_uint32_t flag, zp_uint32_t bit ) -> zp_bool_t
{
    const zp_uint32_t test = 1 << bit;
    return ( flag & test ) == test;
}

ZP_FORCEINLINE constexpr auto zp_flag32_all_set( zp_uint32_t flag, zp_uint32_t mask ) -> zp_bool_t
{
    return ( flag & mask ) == mask;
}

ZP_FORCEINLINE constexpr auto zp_flag32_any_set( zp_uint32_t flag, zp_uint32_t mask ) -> zp_bool_t
{
    return ( flag & mask ) != 0;
}

//
//
//

template<typename T, typename Eq>
constexpr const T* zp_find( const T* begin, const T* end, Eq eq )
{
    const T* found = nullptr;

    for( ; begin != end; ++begin )
    {
        if( eq( *begin ) )
        {
            found = begin;
            break;
        }
    }

    return found;
}

template<typename T, typename Eq>
constexpr const T* zp_find( const T* begin, const T* end, const T& value, Eq eq )
{
    const T* found = nullptr;

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
constexpr const T* zp_find( const T* begin, const T* end, const T& value )
{
    const T* found = zp_find( begin, end, value, []( const T& lh, const T& rh ) -> zp_bool_t
    {
        return lh == rh;
    } );
    return found;
}

template<typename T, typename Eq>
constexpr zp_size_t zp_find_index( const T* begin, const T* end, Eq eq )
{
    const T* found = zp_find( begin, end, eq );
    return found == nullptr ? zp::npos : found - begin;
}

template<typename T, typename Eq>
constexpr zp_size_t zp_find_index( const T* begin, const T* end, const T& value, Eq eq )
{
    T* found = zp_find( begin, end, value, eq );
    return found == nullptr ? zp::npos : found - begin;
}

template<typename T, typename Eq>
constexpr zp_size_t zp_index_of( T* begin, T* end, const T& value )
{
    T* found = zp_find( begin, end, value );
    return found == nullptr ? zp::npos : found - begin;
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
constexpr zp_size_t zp_find_index( const T ( &arr )[ Size ], const T& value, Eq eq )
{
    zp_size_t index = zp::npos;

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
constexpr zp_bool_t zp_try_find_index( const T ( &arr )[ Size ], const T& value, Eq eq, zp_size_t& index )
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

namespace zp
{
    struct SizeInfo
    {
        zp_float32_t size;
        zp_char8_t mem[ 4 ];
    };

    constexpr SizeInfo GetSizeInfoFromBytes( zp_size_t bytes, zp_bool_t asBits = false )
    {
        SizeInfo info {};

        if( asBits )
        {
            info.mem[ 1 ] = 'b';
            if( bytes > 1 Tb )
            {
                info.size = (zp_float32_t)( (zp_float64_t)bytes / ( 1 Tb ) );
                info.mem[ 0 ] = 't';
            }
            else if( bytes > 1 Gb )
            {
                info.size = (zp_float32_t)bytes / (zp_float32_t)( 1 Gb );
                info.mem[ 0 ] = 'g';
            }
            else if( bytes > 1 Mb )
            {
                info.size = (zp_float32_t)bytes / ( 1 Mb );
                info.mem[ 0 ] = 'm';
            }
            else if( bytes > 1 Kb )
            {
                info.size = (zp_float32_t)bytes / ( 1 Kb );
                info.mem[ 0 ] = 'k';
            }
            else
            {
                info.size = (zp_float32_t)bytes;
                info.mem[ 0 ] = 'b';
                info.mem[ 1 ] = '\0';
            }
        }
        else
        {
            info.mem[ 1 ] = 'B';
            if( bytes > 1 TB )
            {
                info.size = (zp_float32_t)( (zp_float64_t)bytes / ( 1 TB ) );
                info.mem[ 0 ] = 'T';
            }
            else if( bytes > 1 GB )
            {
                info.size = (zp_float32_t)bytes / (zp_float32_t)( 1 GB );
                info.mem[ 0 ] = 'G';
            }
            else if( bytes > 1 MB )
            {
                info.size = (zp_float32_t)bytes / ( 1 MB );
                info.mem[ 0 ] = 'M';
            }
            else if( bytes > 1 KB )
            {
                info.size = (zp_float32_t)bytes / ( 1 KB );
                info.mem[ 0 ] = 'K';
            }
            else
            {
                info.size = (zp_float32_t)bytes;
                info.mem[ 0 ] = 'B';
                info.mem[ 1 ] = '\0';
            }
        }

        return info;
    }
}

#endif //ZP_COMMON_H
