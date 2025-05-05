//
// Created by phosg on 7/4/2023.
//

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Memory.h"
#include "Core/String.h"

namespace zp
{
    MutableString::MutableString( MutableString::str_pointer str, zp_size_t capacity )
        : m_str( str )
        , m_length( 0 )
        , m_capacity( capacity )
    {
    }

    MutableString::char_type& MutableString::operator[]( zp_size_t index )
    {
        ZP_ASSERT( m_str != nullptr && index < m_length );
        return m_str[ index ];
    }

    zp_bool_t MutableString::empty() const
    {
        return zp_strempty( m_str, m_length );
    }

    zp_size_t MutableString::length() const
    {
        return m_length;
    }

    zp_size_t MutableString::capacity() const
    {
        return m_capacity;
    }

    const char* MutableString::c_str() const
    {
        return reinterpret_cast<const char*>(m_str);
    }

    MutableString::str_pointer MutableString::str()
    {
        return m_str;
    }

    MutableString::str_pointer MutableString::data()
    {
        return m_str;
    }

    MutableString::const_str_pointer MutableString::data() const
    {
        return m_str;
    }

    MutableString::iterator MutableString::begin()
    {
        return m_str;
    }

    MutableString::iterator MutableString::end()
    {
        return m_str + m_length;
    }

    MutableString::const_iterator MutableString::begin() const
    {
        return m_str;
    }

    MutableString::const_iterator MutableString::end() const
    {
        return m_str + m_length;
    }

    void MutableString::clear()
    {
        ZP_ASSERT( m_str != nullptr );
        m_length = 0;
        m_str[ 0 ] = '\0';
    }

    void MutableString::append( char_type ch )
    {
        ZP_ASSERT( m_str != nullptr );
        m_str[ m_length ] = ch;
        m_length++;
        m_str[ m_length ] = '\0';
    }

    void MutableString::append( const char* str )
    {
        ZP_ASSERT( m_str != nullptr );
        for( ; m_length < m_capacity && *str != '\0'; ++m_length, ++str )
        {
            m_str[ m_length ] = *str;
        }
        m_str[ m_length ] = '\0';
    }

    void MutableString::append( const char* str, zp_size_t length )
    {
        ZP_ASSERT( m_str != nullptr );
        const char* end = str + length;
        for( ; length < m_capacity && *str != '\0' && str != end; ++length, ++str )
        {
            m_str[ m_length ] = *str;
        }
        m_str[ m_length ] = '\0';
    }

    void MutableString::append( const String& str )
    {
        append( str.c_str(), str.length() );
    }

    zp_bool_t MutableString::operator=( const zp::MutableString& other ) const
    {
        return m_str == other.m_str || zp_strcmp( m_str, m_length, other.m_str, other.m_length ) == 0;
    }

    zp_bool_t MutableString::operator=( const zp::String& other ) const
    {
        return m_str == other.data() || zp_strcmp( m_str, m_length, other.data(), other.length() ) == 0;
    }
}

namespace zp
{
    Tokenizer::Tokenizer( const String& str, const char* delim )
        : m_str( str )
        , m_delim( String::As( delim ) )
        , m_next( 0 )
    {
    }

    Tokenizer::Tokenizer( const char* str, zp_size_t length, const char* delim )
        : m_str( String::As( str, length ) )
        , m_delim( String::As( delim ) )
        , m_next( 0 )
    {
    }

    zp_size_t Tokenizer::position() const
    {
        return m_next;
    }

    String Tokenizer::remaining() const
    {
        return String::As( m_str.c_str() + m_next, m_str.length() - m_next );
    }


    zp_bool_t Tokenizer::next( String& token )
    {
        token = {};

        zp_bool_t hasNext = false;
        if( m_next < m_str.length() )
        {
#if 1
            zp_bool_t fullDelim = true;
            const zp_char8_t* end = m_str.str() + m_next;

            while( *end != '\0' && end < m_str.end() )
            {
                zp_bool_t isDelim = false;
                if( fullDelim )
                {
                    isDelim = zp_strcmp( end, m_delim.length(), m_delim.str(), m_delim.length() ) == 0;
                }
                else
                {
                    for( zp_size_t i = 0; i < m_delim.length() && !isDelim; ++i )
                    {
                        isDelim = m_delim[ i ] == *end;
                    }
                }

                if( isDelim )
                {
                    break;
                }

                ++end;
            }

            const zp_char8_t* front = m_str.str() + m_next;
            token = { front, static_cast<zp_size_t>(end - front) };
            m_next += token.length() + ( fullDelim ? m_delim.length() : 0 );

            hasNext = m_next <= m_str.length();
#else
            const zp_char8_t* front = m_str.str() + m_next;
            const zp_char8_t* end = m_str.str() + m_next;

            zp_size_t idx = front - m_str.str();

            while( *end != '\0' && idx < m_str.length() )
            {
                zp_bool_t isDelim = true;
                for( zp_size_t i = 0; i < m_delim.length() && isDelim; ++i )
                {
                    isDelim &= m_delim[ i ] == *end;
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

            token = { front, static_cast<zp_size_t>(end - front) };

            hasNext = ( end - m_str.str() ) <= m_str.length();

            while( *end != '\0' && idx < m_str.length() )
            {
                zp_bool_t isDelim = false;
                for( zp_size_t i = 0; i < m_delim.length() && !isDelim; ++i )
                {
                    isDelim = m_delim[ i ] == *end;
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

            m_next = end - m_str.str();
#endif
        }

        return hasNext;
    }

    void Tokenizer::reset()
    {
        m_next = 0;
    }
}

//
//
//

zp_int32_t zp_strcmp( const zp::String& lh, const zp::String& rh )
{
    return zp_strcmp( lh.str(), lh.length(), rh.str(), rh.length() );
}

const char* zp_strnstr( const zp::String& str, const zp::String& find )
{
    return zp_strnstr( str.c_str(), str.length(), find.c_str(), find.length() );
}
