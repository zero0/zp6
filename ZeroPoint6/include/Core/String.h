//
// Created by phosg on 7/4/2023.
//

#ifndef ZP_STRING_H
#define ZP_STRING_H

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Math.h"
#include "Core/Allocator.h"

constexpr zp_size_t zp_strlen( const char* str );

//
//
//

zp_int32_t zp_strcmp( const char* lh, const char* rh );

zp_int32_t zp_strcmp( const char* lh, zp_size_t lhSize, const char* rh, zp_size_t rhSize );

zp_int32_t zp_strcmp( const zp_char8_t* lh, zp_size_t lhSize, const zp_char8_t* rh, zp_size_t rhSize );

template<zp_size_t RHSize>
zp_int32_t zp_strcmp( const char* lh, zp_size_t lhSize, const char (& rh)[RHSize] )
{
    return zp_strcmp( lh, lhSize, rh, zp_strlen( rh ) );
}

template<zp_size_t LHSize>
zp_int32_t zp_strcmp( const char (& lh)[LHSize], const char* rh, zp_size_t rhSize )
{
    return zp_strcmp( lh, zp_strlen( lh ), rh, rhSize );
}

//zp_size_t zp_strlen( const char* str );

//zp_size_t zp_strnlen( const char* str, zp_size_t maxSize );

constexpr void zp_strcpy( char* dstStr, zp_size_t dstLength, const char* srcStr )
{
    zp_size_t i;
    for( i = 0; srcStr[ i ] != '\0' && i < dstLength; ++i )
    {
        dstStr[ i ] = srcStr[ i ];
    }
    dstStr[ i ] = '\0';
}

constexpr void zp_strcpy( zp_char8_t* dstStr, zp_size_t dstLength, const zp_char8_t* srcStr )
{
    zp_size_t i;
    for( i = 0; srcStr[ i ] != '\0' && i < dstLength; ++i )
    {
        dstStr[ i ] = srcStr[ i ];
    }
    dstStr[ i ] = '\0';
}

constexpr void zp_strncpy( char* dstStr, zp_size_t dstLength, const char* srcStr, zp_size_t srcLen )
{
    zp_size_t i;
    for( i = 0; srcStr[ i ] != '\0' && i < srcLen && i < dstLength; ++i )
    {
        dstStr[ i ] = srcStr[ i ];
    }
    dstStr[ i ] = '\0';
}

constexpr void zp_strncpy( zp_char8_t* dstStr, zp_size_t dstLength, const zp_char8_t* srcStr, zp_size_t srcLen )
{
    zp_size_t i;
    for( i = 0; srcStr[ i ] != '\0' && i < srcLen && i < dstLength; ++i )
    {
        dstStr[ i ] = srcStr[ i ];
    }
    dstStr[ i ] = '\0';
}

template<zp_size_t Size>
constexpr void zp_strncpy( char (& dstStr)[Size], const char* srcStr, zp_size_t srcLen )
{
    zp_strncpy( dstStr, Size, srcStr, srcLen );
}

template<zp_size_t Size>
constexpr void zp_strncpy( zp_char8_t (& dstStr)[Size], const zp_char8_t* srcStr, zp_size_t srcLen )
{
    zp_strncpy( dstStr, Size, srcStr, srcLen );
}

template<zp_size_t Size>
constexpr void zp_strncpy( zp_char8_t (& dstStr)[Size], const char* srcStr, zp_size_t srcLen )
{
    zp_strncpy( dstStr, Size, reinterpret_cast<const zp_char8_t*>(srcStr), srcLen );
}

constexpr zp_bool_t zp_strempty( const char* str )
{
    return !( str != nullptr && str[ 0 ] != '\0' );
}

constexpr zp_bool_t zp_strempty( const zp_char8_t* str );

constexpr zp_size_t zp_strlen( const char* str )
{
    const char* s = str;
    if( s )
    {
        while( *s )
        {
            ++s;
        }
    }

    const zp_size_t length = s - str;
    return length;
}

constexpr zp_size_t zp_strlen( const char8_t* str )
{
    zp_size_t length = 0;
    if( str )
    {
        while( *str )
        {
            ++str;
            ++length;
        }
    }

    return length;
}

constexpr zp_size_t zp_strnlen( const char* str, zp_size_t maxSize )
{
    zp_size_t length = 0;
    if( str )
    {
        while( *str && length < maxSize )
        {
            ++str;
            ++length;
        }
    }

    return length;
}

constexpr const char* zp_strchr( const char* str, char ch )
{
    const char* start = str;

    if( start )
    {
        while( *start && *start != ch )
        {
            ++start;
        }
    }

    return start;
}

constexpr char* zp_strchr( char* str, char ch )
{
    char* start = str;

    if( start )
    {
        while( *start && *start != ch )
        {
            ++start;
        }
    }

    return start;
}

constexpr zp_size_t zp_rtrim( const char* str, zp_size_t length )
{
    zp_size_t i;
    for( i = length; i > 0; --i )
    {
        switch( str[ i - 1 ] )
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                continue;
            default:
                break;
        }
    }

    return i;
}

constexpr const char* zp_ltrim( const char* str, zp_size_t length )
{
    zp_size_t i;
    for( i = 0; i < length; ++i )
    {
        switch( str[ i ] )
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                continue;
            default:
                break;
        }
    }

    return str + i;
}

constexpr zp_size_t zp_rtrim( const zp_char8_t* str, zp_size_t length )
{
    zp_size_t i;
    for( i = length; i > 0; --i )
    {
        switch( str[ i - 1 ] )
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                continue;
            default:
                return i;
        }
    }

    return i;
}

constexpr const zp_char8_t* zp_ltrim( const zp_char8_t* str, zp_size_t length )
{
    zp_size_t i;
    for( i = 0; i < length; ++i )
    {
        switch( str[ i ] )
        {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                continue;
            default:
                return str + i;
        }
    }

    return str + i;
}

constexpr const char* zp_strnstr( const char* str, zp_size_t strLen, const char* find, zp_size_t findLen )
{
    const char* start = nullptr;

    if( str && find )
    {
        zp_size_t i = 0;
        zp_size_t f = 0;

        for( ; i < strLen && f < findLen; ++i )
        {
            if( str[ i ] == find[ f ] )
            {
                ++f;
                if( f == findLen )
                {
                    ++i;
                    break;
                }
            }
            else
            {
                f = 0;
            }
        }

        if( findLen > 0 && f == findLen )
        {
            start = str + ( i - f );
        }
    }

    return start;
}

template<zp_size_t Size>
constexpr const char* zp_strnstr( const char* str, zp_size_t strLen, const char (& find)[Size] )
{
    return zp_strnstr( str, strLen, find, zp_strlen( find ) );
}

constexpr const char* zp_strstr( const char* str, const char* find )
{
    return zp_strnstr( str, zp_strlen( str ), find, zp_strlen( find ) );
}

constexpr const char* zp_strrchr( const char* str, char ch )
{
    const zp_size_t len = zp_strlen( str );

    const char* end = str;
    if( len > 0 )
    {
        end += len - 1;
        for( ; end != str && *end != ch; --end )
        {
            // noop
        }
    }

    return end;
}

constexpr char* zp_strrchr( char* str, char ch )
{
    const zp_size_t len = zp_strlen( str );

    char* end = str;
    if( len > 0 )
    {
        end += len - 1;
        for( ; end != str && *end != ch; --end )
        {
            // noop
        }
    }

    return end;
}

constexpr zp_uint32_t zp_char_to_uint32( char c )
{
    zp_uint32_t v;
    switch( c )
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            v = (zp_uint32_t)( c - '0' );
            break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            v = ( (zp_uint32_t)( c - 'a' ) + 10 );
            break;

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            v = ( (zp_uint32_t)( c - 'A' ) + 10 );
            break;

        default:
            v = 0;
    }
    return v;
}

constexpr zp_bool_t zp_try_parse_uint8( const char* str, zp_size_t length, zp_uint8_t* m )
{
    zp_uint32_t value = 0;
    zp_uint32_t v = 0;
    for( zp_int32_t i = 0; i < length && i < 2; ++i )
    {
        const char c = str[ i ];
        switch( c )
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                v = (zp_uint32_t)( c - '0' );
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                v = ( (zp_uint32_t)( c - 'a' ) + 10 );
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                v = ( (zp_uint32_t)( c - 'A' ) + 10 );
                break;

            default:
                return false;
        }

        value |= ( v & 0x0F ) << ( ( ( length - 1 ) - i ) * 4 );
    }

    *m = value;
    return true;
}

constexpr zp_bool_t zp_try_parse_uint32( const char* str, zp_uint32_t* m )
{
    zp_uint32_t value = 0;
    zp_uint32_t v;
    for( zp_int32_t i = 0; i < 8; ++i )
    {
        const char c = str[ i ];
        switch( c )
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                v = (zp_uint32_t)( c - '0' );
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                v = ( (zp_uint32_t)( c - 'a' ) + 10 );
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                v = ( (zp_uint32_t)( c - 'A' ) + 10 );
                break;

            default:
                return false;
        }

        value |= ( v & 0x0F ) << ( ( 7 - i ) * 4 );
    }

    *m = value;
    return true;
}

constexpr zp_bool_t zp_try_parse_uint32( const char* str, zp_size_t length, zp_uint32_t* m )
{
    zp_uint32_t value = 0;
    zp_uint32_t v = 0;
    for( zp_int32_t i = 0; i < length && i < 8; ++i )
    {
        const char c = str[ i ];
        switch( c )
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                v = (zp_uint32_t)( c - '0' );
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                v = ( (zp_uint32_t)( c - 'a' ) + 10 );
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                v = ( (zp_uint32_t)( c - 'A' ) + 10 );
                break;

            default:
                return false;
        }

        value |= ( v & 0x0F ) << ( ( ( length - 1 ) - i ) * 4 );
    }

    *m = value;
    return true;
}

constexpr zp_bool_t zp_try_uint32_to_string( const zp_uint32_t m, char* str )
{
    char lit;
    for( zp_int32_t i = 0; i < 8; ++i )
    {
        const zp_uint32_t cur = ( m >> ( ( 7 - i ) * 4 ) ) & 0x0F;
        switch( cur )
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                lit = (char)( '0' + cur );
                break;

            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
                lit = (char)( 'a' + ( cur - 10 ) );
                break;

            default:
                return false;
        }

        str[ i ] = lit;
    }

    return true;
}

constexpr zp_bool_t zp_try_parse_hash64( const char* str, zp_hash64_t* hash )
{
    const zp_size_t length = zp_strlen( str );
    zp_bool_t ok = length == 16;
    if( ok )
    {
        zp_uint32_t m1, m0;
        ok = zp_try_parse_uint32( str + 0, &m1 );
        ok = ok && zp_try_parse_uint32( str + 8, &m0 );

        if( ok )
        {
            *hash = ( (zp_uint64_t)m1 << 32 ) | m0;
        }
    }

    return ok;
}

constexpr zp_bool_t zp_try_hash64_to_string( const zp_hash64_t& hash, char* str )
{
    zp_bool_t ok;
    const zp_uint32_t m1 { (zp_uint32_t)( hash >> 32 ) };
    const zp_uint32_t m0 { (zp_uint32_t)hash };

    ok = zp_try_uint32_to_string( m1, str + 0 );
    ok = ok && zp_try_uint32_to_string( m0, str + 8 );
    *( str + 16 ) = '\0';

    return ok;
}

constexpr zp_bool_t zp_try_parse_hash128( const char* str, zp_size_t length, zp_hash128_t* hash )
{
    zp_bool_t ok = length >= 32;
    if( ok )
    {
        zp_uint32_t m3, m2, m1, m0;
        ok = zp_try_parse_uint32( str + 0, &m3 );
        ok = ok && zp_try_parse_uint32( str + 8, &m2 );
        ok = ok && zp_try_parse_uint32( str + 16, &m1 );
        ok = ok && zp_try_parse_uint32( str + 24, &m0 );

        if( ok )
        {
            hash->m32 = ( (zp_uint64_t)m3 << 32 ) | m2;
            hash->m10 = ( (zp_uint64_t)m1 << 32 ) | m0;
            //hash->m3 = m3;
            //hash->m2 = m2;
            //hash->m1 = m1;
            //hash->m0 = m0;
        }
    }

    return ok;
}

constexpr zp_bool_t zp_try_parse_hash128( const char* str, zp_hash128_t* hash )
{
    const zp_size_t length = zp_strlen( str );
    return zp_try_parse_hash128( str, length, hash );
}

constexpr zp_bool_t zp_try_hash128_to_string( const zp_hash128_t& hash, char* str )
{
    zp_bool_t ok;
    ok = zp_try_uint32_to_string( (zp_uint32_t)( hash.m32 >> 32 ), str + 0 );
    ok = ok && zp_try_uint32_to_string( (zp_uint32_t)hash.m32, str + 8 );
    ok = ok && zp_try_uint32_to_string( (zp_uint64_t)( hash.m10 >> 32 ), str + 16 );
    ok = ok && zp_try_uint32_to_string( (zp_uint32_t)hash.m10, str + 24 );
    *( str + 32 ) = '\0';

    return ok;
}

constexpr zp_bool_t zp_try_parse_guid128( const char* str, zp_guid128_t* guid )
{
    const zp_size_t length = zp_strlen( str );
    zp_bool_t ok = length >= 32;
    if( ok )
    {
        zp_uint32_t m3, m2, m1, m0;
        ok = zp_try_parse_uint32( str + 0, &m3 );
        ok = ok && zp_try_parse_uint32( str + 8, &m2 );
        ok = ok && zp_try_parse_uint32( str + 16, &m1 );
        ok = ok && zp_try_parse_uint32( str + 24, &m0 );

        if( ok )
        {
            guid->m32 = ( (zp_uint64_t)m3 << 32 ) | m2;
            guid->m10 = ( (zp_uint64_t)m1 << 32 ) | m0;
            //guid->m3 = m3;
            //guid->m2 = m2;
            //guid->m1 = m1;
            //guid->m0 = m0;
        }
    }

    return ok;
}

constexpr zp_bool_t zp_try_guid128_to_string( const zp_guid128_t& guid, char* str )
{
    zp_bool_t ok;
    ok = zp_try_uint32_to_string( (zp_uint32_t)( guid.m32 >> 32 ), str + 0 );
    ok = ok && zp_try_uint32_to_string( (zp_uint32_t)guid.m32, str + 8 );
    ok = ok && zp_try_uint32_to_string( (zp_uint64_t)( guid.m10 >> 32 ), str + 16 );
    ok = ok && zp_try_uint32_to_string( (zp_uint32_t)guid.m10, str + 24 );
    *( str + 32 ) = '\0';

    return ok;
}

//
//
//

namespace zp
{
    struct String
    {
        const zp_char8_t* str;
        zp_size_t length;

        ZP_FORCEINLINE static String As( const char* c_str )
        {
            return {
                .str = reinterpret_cast<const zp_char8_t*>(c_str),
                .length = zp_strlen( c_str )
            };
        }

        ZP_FORCEINLINE static String As( const char* c_str, zp_size_t length )
        {
            return {
                .str = reinterpret_cast<const zp_char8_t*>(c_str),
                .length = length
            };
        }

        [[nodiscard]] zp_char8_t operator[]( zp_size_t index ) const
        {
            return str && index < length ? str[ index ] : '\0';
        }

        [[nodiscard]] zp_bool_t empty() const
        {
            return !( str && length );
        }

        [[nodiscard]] const char* c_str() const
        {
            return reinterpret_cast<const char*>(str);
        }
    };

    constexpr zp_bool_t operator==( const String& lh, const String& rh )
    {
        return lh.str == rh.str || zp_strcmp( lh.str, lh.length, rh.str, rh.length ) == 0;
    }

    template<typename H>
    struct DefaultHash<String, H>
    {
        H operator()( const String& val ) const
        {
            return zp_fnv_1a<H>( val.str, val.length );
        }
    };


    struct MutableString
    {
        zp_char8_t* const str;
        zp_size_t length;
        const zp_size_t capacity;

        [[nodiscard]] zp_bool_t empty() const
        {
            return !( str && length );
        }
    };

    //
    //
    //
#pragma region Alloc String

    class AllocString
    {
    public:
        AllocString()
            : m_str( nullptr )
            , m_length( 0 )
            , memoryLabel( 0 )
        {
        }

        AllocString( MemoryLabel memoryLabel, const zp_char8_t* str )
            : m_str( nullptr )
            , m_length( zp_strlen( str ) )
            , memoryLabel( memoryLabel )
        {
            if( m_length > 0 )
            {
                m_str = ZP_MALLOC_T_ARRAY( memoryLabel, zp_char8_t, m_length + 1 );
                zp_strcpy( m_str, m_length, str );
            }
        }

        AllocString( MemoryLabel memoryLabel, const zp_char8_t* str, zp_size_t length )
            : m_str( nullptr )
            , m_length( length )
            , memoryLabel( memoryLabel )
        {
            if( m_length > 0 )
            {
                m_str = ZP_MALLOC_T_ARRAY( memoryLabel, zp_char8_t, m_length + 1 );
                zp_strcpy( m_str, m_length, str );
            }
        }

        AllocString( MemoryLabel memoryLabel, const char* str )
            : m_str( nullptr )
            , m_length( zp_strlen( str ) )
            , memoryLabel( memoryLabel )
        {
            if( m_length > 0 )
            {
                m_str = ZP_MALLOC_T_ARRAY( memoryLabel, zp_char8_t, m_length + 1 );
                zp_strcpy( m_str, m_length, reinterpret_cast<const zp_char8_t*>( str ) );
            }
        }

        AllocString( MemoryLabel memoryLabel, const char* str, zp_size_t length )
            : m_str( nullptr )
            , m_length( length )
            , memoryLabel( memoryLabel )
        {
            if( m_length > 0 )
            {
                m_str = ZP_MALLOC_T_ARRAY( memoryLabel, zp_char8_t, m_length + 1 );
                zp_strcpy( m_str, m_length, reinterpret_cast<const zp_char8_t*>( str ) );
            }
        }

        ~AllocString()
        {
            ZP_SAFE_FREE( memoryLabel, m_str );
            m_length = 0;
        }

        AllocString( const AllocString& other )
            : m_str( nullptr )
            , m_length( other.m_length )
            , memoryLabel( other.memoryLabel )
        {
            if( m_length > 0 )
            {
                m_str = ZP_MALLOC_T_ARRAY( memoryLabel, zp_char8_t, m_length + 1 );
                zp_strcpy( m_str, m_length, other.m_str );
            }
        }

        AllocString( AllocString&& other ) noexcept
            : m_str( other.m_str )
            , m_length( other.m_length )
            , memoryLabel( other.memoryLabel )
        {
            other.m_str = nullptr;
            other.m_length = 0;
        }

        AllocString& operator=( const AllocString& other )
        {
            if( m_str != other.m_str )
            {
                ZP_SAFE_FREE( memoryLabel, m_str );

                m_str = other.m_str;
                m_length = other.m_length;
            }

            return *this;
        }

        AllocString& operator=( AllocString&& other ) noexcept
        {
            ZP_SAFE_FREE( memoryLabel, m_str );

            m_str = other.m_str;
            m_length = other.m_length;

            other.m_str = nullptr;
            other.m_length = 0;

            return *this;
        }

        [[nodiscard]] zp_char8_t operator[]( zp_size_t index ) const
        {
            return m_str && index < m_length ? m_str[ index ] : '\0';
        }

        [[nodiscard]] zp_bool_t empty() const
        {
            return !( m_str && m_length );
        }

        [[nodiscard]] zp_size_t length() const
        {
            return m_length;
        }

        [[nodiscard]] const char* c_str() const
        {
            return reinterpret_cast<const char*>( m_str);
        }

        [[nodiscard]] const zp_char8_t* str() const
        {
            return m_str;
        }

        [[nodiscard]] String asString() const
        {
            return { .str = m_str, .length = m_length };
        }

        void set( MemoryLabel memoryLabel, zp_char8_t* str, zp_size_t length )
        {
            ZP_SAFE_FREE( memoryLabel, m_str );

            this->memoryLabel = memoryLabel;
            m_str = str;
            m_length = length;
        }
//
        //explicit operator MutableString()
        //{
        //    return { .str = m_str, .length = length, .capacity = m_capacity };
        //}

    private:
        zp_char8_t* m_str;
        zp_size_t m_length;

    public:
        MemoryLabel memoryLabel;

        friend constexpr zp_bool_t operator==( const AllocString& lh, const AllocString& rh );
    };

    constexpr zp_bool_t operator==( const AllocString& lh, const AllocString& rh )
    {
        return lh.m_str == rh.m_str || zp_strcmp( lh.m_str, lh.m_length, rh.m_str, rh.m_length ) == 0;
    }

    template<typename H>
    struct DefaultHash<AllocString, H>
    {
        H operator()( const AllocString& val ) const
        {
            return zp_fnv_1a<H>( val.str(), val.length() );
        }
    };


#pragma endregion

#pragma region Fixed String

    template<zp_size_t Size>
    class FixedString
    {
    public:
        FixedString()
            : m_str()
        {
        }

        explicit FixedString( const char* str )
            : m_str()
        {
            const zp_size_t length = zp_strlen( str );
            if( str && length )
            {
                zp_strncpy( m_str, str, length );
            }
        }

        FixedString( const char* str, zp_size_t length )
            : m_str()
        {
            if( str && length )
            {
                zp_strncpy( m_str, str, length );
            }
        }

        FixedString( const FixedString<Size>& other )
            : m_str()
        {
            zp_strncpy( m_str, other.str(), other.length() );
        }

        FixedString( FixedString<Size>&& other ) noexcept
            : m_str()
        {
            zp_strncpy( m_str, other.str(), other.length() );
        }

        ~FixedString() = default;

        [[nodiscard]] const zp_char8_t& operator[]( zp_size_t index ) const
        {
            return m_str[ index ];
        }

        [[nodiscard]] zp_bool_t empty() const
        {
            return m_str[ 0 ] == '\0';
        }

        [[nodiscard]] zp_size_t length() const
        {
            return zp_strlen( m_str );
        }

        [[nodiscard]] zp_size_t capacity() const
        {
            return Size;
        }

        [[nodiscard]] const char* c_str() const
        {
            return reinterpret_cast<const char*>( m_str );
        }

        [[nodiscard]] const zp_char8_t* str() const
        {
            return m_str;
        }

        [[nodiscard]] explicit operator String() const
        {
            return { .str = m_str, .length = length() };
        }

        FixedString<Size>& operator=( const FixedString<Size>& other ) noexcept
        {
            if( m_str != other.m_str )
            {
                zp_strncpy( m_str, other.m_str, other.length() );
            }
            return *this;
        }

        FixedString<Size>& operator=( FixedString<Size>&& other ) noexcept
        {
            zp_strncpy( m_str, other.m_str, other.length() );
            return *this;
        }

        FixedString<Size>& operator=( const String& other ) noexcept
        {
            zp_strncpy( m_str, other.str, other.length );
            return *this;
        }

        FixedString<Size>& operator=( String&& other ) noexcept
        {
            zp_strncpy( m_str, other.str, other.length );
            return *this;
        }

        FixedString<Size>& operator=( const char* other ) noexcept
        {
            zp_strncpy( m_str, other, zp_strlen( other ) );
            return *this;
        }

        FixedString<Size>& operator=( const zp_char8_t* other ) noexcept
        {
            zp_strncpy( m_str, other, zp_strlen( other ) );
            return *this;
        }

    private:
        zp_char8_t m_str[Size];
    };

    template<zp_size_t Size>
    class MutableFixedString
    {
    public:
        MutableFixedString()
            : m_str()
            , m_length( 0 )
        {
        }

        explicit MutableFixedString( const char* str )
            : m_str()
            , m_length( zp_strlen( str ) )
        {
            if( str && m_length )
            {
                zp_strcpy( str, m_str, zp_min( Size, m_length ) );
            }
        }

        MutableFixedString( const char* str, zp_size_t length )
            : m_str()
            , m_length( length )
        {
            if( str && length )
            {
                zp_strcpy( str, m_str, zp_min( m_length, length ) );
            }
        }

        ~MutableFixedString() = default;

        [[nodiscard]] const zp_char8_t& operator[]( zp_size_t index ) const
        {
            return m_str[ index ];
        }

        [[nodiscard]] zp_char8_t& operator[]( zp_size_t index )
        {
            return m_str[ index ];
        }

        [[nodiscard]] zp_size_t length() const
        {
            return m_length;
        }

        [[nodiscard]] zp_size_t capacity() const
        {
            return Size;
        }

        [[nodiscard]] const char* c_str() const
        {
            return reinterpret_cast<const char*>( m_str );
        }

        [[nodiscard]] const zp_char8_t* str() const
        {
            return m_str;
        }

        void clear()
        {
            m_length = 0;
            m_str[ 0 ] = '\0';
        }


        void append( char ch )
        {
            m_str[ m_length ] = ch;
            m_length++;
            m_str[ m_length ] = '\0';
        }

        void append( zp_char8_t ch )
        {
            m_str[ m_length ] = ch;
            m_length++;
            m_str[ m_length ] = '\0';
        }

        void append( const char* str )
        {
            for( ; m_length < Size && *str; ++m_length, ++str )
            {
                m_str[ m_length ] = *str;
            }
            m_str[ m_length ] = '\0';
        }

        void append( const char* str, zp_size_t length )
        {
            const char* end = str + length;
            for( ; m_length < Size && *str && str != end; ++m_length, ++str )
            {
                m_str[ m_length ] = *str;
            }
            m_str[ m_length ] = '\0';
        }

        template<class ... Args>
        void format( const char* format, Args ... args )
        {
            const zp_int32_t len = zp_snprintf( m_str, Size, format, args... );
            m_length = len;
        }

        template<class ... Args>
        void appendFormat( const char* format, Args ... args )
        {
            const zp_int32_t len = zp_snprintf( m_str + m_length, Size - m_length, format, args... );
            m_length += len;
        }

        [[nodiscard]] operator String() const
        {
            return { .str = m_str, .length = m_length };
        }
//
        //explicit operator MutableString()
        //{
        //    return { .str = m_str, .length = length, .capacity = Size };
        //}

    private:
        zp_char8_t m_str[Size];
        zp_size_t m_length;
    };

#pragma endregion

#pragma region Pooled String

    class PooledString
    {
    public:

    private:
        zp_char8_t* m_str;

    public:
        ZP_FORCEINLINE zp_bool_t operator==( const PooledString& rh )
        {
            return m_str == rh.m_str;
        }
    };

#pragma endregion

    class ConstantString
    {

    };

    class Tokenizer
    {
    public:
        Tokenizer( const char* str, zp_size_t length, const char* delim )
            : m_str( String::As( str, length ) )
            , m_delim( String::As( delim ) )
            , m_next( 0 )
        {
        }

        zp_bool_t next( String& token )
        {
            token.str = nullptr;
            token.length = 0;

            zp_bool_t hasNext = false;
            if( m_next < m_str.length )
            {
                const zp_char8_t* front = m_str.str + m_next;
                const zp_char8_t* end = m_str.str + m_next;

                zp_size_t idx = front - m_str.str;

                while( *end && idx < m_str.length )
                {
                    zp_bool_t isDelim = false;
                    for( zp_size_t i = 0; i < m_delim.length && !isDelim; ++i )
                    {
                        isDelim = m_delim.str[ i ] == *end;
                    }

                    if( isDelim )
                    {
                        break;
                    }
                    else
                    {
                        ++end;
                        ++idx;
                    }
                }

                token.str = front;
                token.length = end - front;

                hasNext = ( end - m_str.str ) <= m_str.length;

                while( *end && idx < m_str.length )
                {
                    zp_bool_t isDelim = false;
                    for( zp_size_t i = 0; i < m_delim.length && !isDelim; ++i )
                    {
                        isDelim = m_delim.str[ i ] == *end;
                    }

                    if( isDelim )
                    {
                        ++end;
                        ++idx;
                    }
                    else
                    {
                        break;
                    }
                }

                m_next = end - m_str.str;
            }

            return hasNext;
        }

        void reset()
        {
            m_next = 0;
        }

    private:
        String m_str;
        String m_delim;

        zp_size_t m_next;
    };
};

#endif //ZP_STRING_H
