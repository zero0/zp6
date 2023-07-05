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

zp_int32_t zp_strcmp( const char* lh, const char* rh );

//zp_size_t zp_strlen( const char* str );

//zp_size_t zp_strnlen( const char* str, zp_size_t maxSize );

constexpr void zp_strcpy( const char* srcStr, char* dstStr, zp_size_t dstLength )
{
    for( zp_size_t i = 0; i < dstLength; ++i )
    {
        dstStr[ i ] = srcStr[ i ];
    }
}

constexpr zp_bool_t zp_strempty( const char* str )
{
    return str == nullptr || str[ 0 ] == 0;
}

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
    const char8_t* s = str;
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

constexpr zp_size_t zp_strnlen( const char* str, zp_size_t maxSize )
{
    zp_size_t length = 0;
    if( str )
    {
        const char* s = str;
        while( *s && length < maxSize )
        {
            ++s;
            ++length;
        }
    }

    return length;
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


constexpr zp_bool_t zp_try_parse_uint32( const char* str, zp_uint32_t* m )
{
    *m = 0;

    zp_uint32_t hex;
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
                hex = (zp_uint32_t)( c - '0' );
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                hex = ( (zp_uint32_t)( c - 'a' ) + 10 );
                break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                hex = ( (zp_uint32_t)( c - 'A' ) + 10 );
                break;

            default:
                return false;
        }

        *m |= ( hex & 0x0F ) << ( ( 7 - i ) * 4 );
    }

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

constexpr zp_bool_t zp_try_parse_hash128( const char* str, zp_hash128_t* hash )
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

namespace zp
{
    struct String
    {
        const zp_char8_t* const str;
        const zp_size_t length;

        [[nodiscard]] zp_bool_t empty() const
        {
            return !( str && length );
        }

        [[nodiscard]] const char* c_str() const
        {
            return reinterpret_cast<const char*>(str);
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

    class BaseString
    {
    public:
        BaseString( zp_size_t capacity, MemoryLabel memoryLabel )
            : m_str( nullptr )
            , m_length( 0 )
            , m_capacity( capacity )
            , memoryLabel( memoryLabel )
        {
        }

        BaseString( const zp_char8_t* str, MemoryLabel memoryLabel )
            : m_str( nullptr )
            , m_length( 0 )
            , m_capacity( 0 )
            , memoryLabel( memoryLabel )
        {
        }

        BaseString( const zp_char8_t* str, zp_size_t length, MemoryLabel memoryLabel )
            : m_str( nullptr )
            , m_length( 0 )
            , m_capacity( length )
            , memoryLabel( memoryLabel )
        {
        }

        [[nodiscard]] zp_bool_t empty() const
        {
            return !( m_str && m_length );
        }

        [[nodiscard]] zp_size_t length() const
        {
            return m_length;
        }

        [[nodiscard]] zp_size_t capacity() const
        {
            return m_capacity;
        }

        [[nodiscard]] const char* c_str() const
        {
            return reinterpret_cast<const char*>( m_str);
        }

        [[nodiscard]] const zp_char8_t* str() const
        {
            const zp_char8_t* ff = ZP_T( "fafaf" );
            return m_str;
        }

        //explicit operator String() const
        //{
        //    return { .str = m_str, .length = m_length };
        //}
//
        //explicit operator MutableString()
        //{
        //    return { .str = m_str, .length = m_length, .capacity = m_capacity };
        //}

    private:
        zp_char8_t* m_str;
        zp_size_t m_length;
        zp_size_t m_capacity;

    public:
        const MemoryLabel memoryLabel;
    };

#pragma region Fixed String

    template<zp_size_t Size>
    class FixedString
    {
    public:
        FixedString()
            : m_str( {} )
            , m_length( 0 )
        {
        }

        explicit FixedString( const char* str )
            : m_str( {} )
            , m_length( zp_strlen( str ) )
        {
            if( str && m_length )
            {
                zp_strcpy( str, m_str, zp_min( Size, m_length ) );
            }
        }

        FixedString( const char* str, zp_size_t length )
            : m_str( {} )
            , m_length( length )
        {
            if( str && length )
            {
                zp_strcpy( str, m_str, zp_min( m_length, length ) );
            }
        }

        ~FixedString() = default;

        [[nodiscard]] zp_char8_t operator[]( zp_size_t index ) const
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
            return m_str;
        }

        [[nodiscard]] const zp_char8_t* str() const
        {
            return m_str;
        }

        //explicit operator String() const
        //{
        //    return { .str = m_str, .length = m_length };
        //}
//
        //explicit operator MutableString()
        //{
        //    return { .str = m_str, .length = m_length, .capacity = Size };
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

};

#endif //ZP_STRING_H