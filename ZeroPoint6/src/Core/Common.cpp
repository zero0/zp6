//
// Created by phosg on 1/11/2022.
//

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/String.h"

#if ZP_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#pragma comment( lib, "rpcrt4.lib")

#include <rpc.h>

#endif // ZP_OS_WINDOWS

#if ZP_DEBUG

#include <debugapi.h>

#endif

enum
{
    kPrintBufferSize = 1024 * 32
};

zp_int32_t zp_printf( const char* text, ... )
{
    char szBuff[kPrintBufferSize];
    va_list arg;
    va_start( arg, text );
    const zp_int32_t write = ::_vsnprintf_s( szBuff, kPrintBufferSize, text, arg );
    va_end( arg );

#if ZP_DEBUG
    if( ::IsDebuggerPresent() )
    {
        ::OutputDebugString( szBuff );
    }
#endif

    ::fprintf_s( stdout, szBuff );

    return write;
}

zp_int32_t zp_printfln( const char* text, ... )
{
    char szBuff[kPrintBufferSize];

    va_list arg;
    va_start( arg, text );
    const zp_int32_t write = ::_vsnprintf_s( szBuff, kPrintBufferSize-1, text, arg );
    va_end( arg );

    szBuff[ write ] = '\n';
    szBuff[ write + 1 ] = '\0';

#if ZP_DEBUG
    if( ::IsDebuggerPresent() )
    {
        ::OutputDebugString( szBuff );
    }
#endif

    ::fprintf_s( stdout, szBuff );
    ::fflush( stdout );

    return write + 1;
}

zp_int32_t zp_error_printf( const char* text, ... )
{
    char szBuff[kPrintBufferSize];
    va_list arg;
    va_start( arg, text );
    const zp_int32_t write = ::_vsnprintf_s( szBuff, kPrintBufferSize, text, arg );
    va_end( arg );

#if ZP_DEBUG
    if( ::IsDebuggerPresent() )
    {
        ::OutputDebugString( szBuff );
    }
#endif

    ::fprintf_s( stderr, szBuff );

    return write;
}

zp_int32_t zp_error_printfln( const char* text, ... )
{
    char szBuff[kPrintBufferSize];

    va_list arg;
    va_start( arg, text );
    const zp_int32_t write = ::_vsnprintf_s( szBuff, kPrintBufferSize, text, arg );
    va_end( arg );

    szBuff[ write ] = '\n';
    szBuff[ write + 1 ] = '\0';

#if ZP_DEBUG
    if( ::IsDebuggerPresent() )
    {
        ::OutputDebugString( szBuff );
    }
#endif

    ::fprintf_s( stderr, szBuff );
    ::fflush( stderr );

    return write + 1;
}

zp_int32_t zp_snprintf( char* dest, zp_size_t destSize, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    const zp_int32_t write = ::_vsnprintf_s( dest, destSize, destSize, format, args );
    va_end( args );

    return write;
}

zp_int32_t zp_atoi32( const char* str, zp_int32_t base )
{
    return ::strtol( str, nullptr, base );
}

zp_int64_t zp_atoi64( const char* str, zp_int32_t base )
{
    return ::strtoll( str, nullptr, base );
}

zp_float32_t zp_atof32( const char* str )
{
    return ::strtof( str, nullptr );
}

void zp_memcpy( void* dst, zp_size_t dstLength, const void* src, zp_size_t srcLength )
{
    ::memcpy_s( dst, dstLength, src, srcLength );
}

void zp_memset( void* dst, zp_size_t dstLength, zp_int32_t value )
{
    ::memset( dst, value, dstLength );
}

zp_int32_t zp_memcmp( const void* lh, zp_size_t lhLength, const void* rh, zp_size_t rhLength )
{
    return ::memcmp( lh, rh, zp_min( lhLength, rhLength ) );
}

zp_guid128_t zp_generate_unique_guid128()
{
    zp_uint32_t m3, m2, m1, m0;

#if ZP_OS_WINDOWS
    UUID uuid;
    ::UuidCreate( &uuid );

    m3 = static_cast<zp_uint32_t>( uuid.Data1 );
    m2 = static_cast<zp_uint32_t>( uuid.Data2 << 16 ) | uuid.Data3;
    m1 = uuid.Data4[ 0 ] << 24 | uuid.Data4[ 1 ] << 16 | uuid.Data4[ 2 ] << 8 | uuid.Data4[ 3 ];
    m0 = uuid.Data4[ 4 ] << 24 | uuid.Data4[ 5 ] << 16 | uuid.Data4[ 6 ] << 8 | uuid.Data4[ 7 ];
#endif

    zp_guid128_t guid {
        .m32 = ( static_cast<zp_uint64_t>( m3 ) << 32 ) | m2,
        .m10 = ( static_cast<zp_uint64_t>( m1 ) << 32 ) | m0
    };

    return guid;
}

zp_guid128_t zp_generate_guid128()
{
    zp_uint32_t m3, m2, m1, m0;

#if ZP_OS_WINDOWS
    UUID uuid;
    ::UuidCreateSequential( &uuid );

    m3 = static_cast<zp_uint32_t>( uuid.Data1 );
    m2 = static_cast<zp_uint32_t>( uuid.Data2 << 16 ) | uuid.Data3;
    m1 = uuid.Data4[ 0 ] << 24 | uuid.Data4[ 1 ] << 16 | uuid.Data4[ 2 ] << 8 | uuid.Data4[ 3 ];
    m0 = uuid.Data4[ 4 ] << 24 | uuid.Data4[ 5 ] << 16 | uuid.Data4[ 6 ] << 8 | uuid.Data4[ 7 ];
#endif

    zp_guid128_t guid {
        .m32 = ( static_cast<zp_uint64_t>( m3 ) << 32 ) | m2,
        .m10 = ( static_cast<zp_uint64_t>( m1 ) << 32 ) | m0
    };

    return guid;
}

//
//
//

namespace
{
    enum zp_lzf_config
    {
        /**
         * The number of entries in the hash table. The size is a trade-off between
         * hash collisions (reduced compression) and speed (amount that fits in CPU
         * cache).
         */
        HASH_SIZE = 1 << 14,

        //HASH_CONST = 2777,
        HASH_CONST = 57321, // better compression hash

        /**
         * The maximum number of literals in a chunk (32).
         */
        MAX_LITERAL = 1 << 5,

        /**
         * The maximum offset allowed for a back-reference (8192).
         */
        MAX_OFF = 1 << 13,

        /**
         * The maximum back-reference length (264).
         */
        MAX_REF = ( 1 << 8 ) + ( 1 << 3 ),
    };

    /**
     * Return byte with lower 2 bytes being byte at index, then index+1.
     */
    ZP_FORCEINLINE zp_int32_t zp_lzf_first( const zp_uint8_t* inBuff, zp_size_t inPos )
    {
        return ( inBuff[ inPos ] << 8 ) | ( inBuff[ inPos + 1 ] & 0xFF );
    }

    /**
     * Shift v 1 byte left, add value at index inPos+2.
     */
    ZP_FORCEINLINE zp_int32_t zp_lzf_next( zp_uint8_t v, const zp_uint8_t* inBuff, zp_size_t inPos )
    {
        return ( v << 8 ) | ( inBuff[ inPos + 2 ] & 0xFF );
    }

    /**
     * Compute the address in the hash table.
     */
    ZP_FORCEINLINE zp_size_t zp_lzf_hash( zp_int32_t h )
    {
        return ( ( h * HASH_CONST ) >> 9 ) & ( HASH_SIZE - 1 );
    }
}

zp_size_t zp_lzf_compress( const void* srcBuffer, zp_size_t srcPosition, zp_size_t srcSize, void* dstBuffer, zp_size_t dstPosition )
{
    zp_size_t hashTab[HASH_SIZE];

    const zp_uint8_t* inBuff = static_cast<const zp_uint8_t*>( srcBuffer );
    zp_uint8_t* outBuff = static_cast<zp_uint8_t*>( dstBuffer );

    zp_int32_t literals = 0;

    dstPosition++;

    zp_int32_t future = zp_lzf_first( inBuff, srcPosition );
    zp_size_t off;
    zp_size_t reff;
    zp_size_t maxLen;
    zp_int32_t len;

    while( srcPosition < srcSize - 4 )
    {
        zp_uint8_t p2 = inBuff[ srcPosition + 2 ];
        // next
        future = ( future << 8 ) + ( p2 & 0xFF );
        off = zp_lzf_hash( future );
        reff = hashTab[ off ];
        hashTab[ off ] = srcPosition;
        if( reff < srcPosition
            && reff > 0
            && ( off = srcPosition - reff - 1 ) < MAX_OFF
            && inBuff[ reff + 2 ] == p2
            && inBuff[ reff + 1 ] == static_cast<zp_uint8_t>( future >> 8 )
            && inBuff[ reff ] == static_cast<zp_uint8_t>( future >> 16 ) )
        {
            // match
            maxLen = srcSize - srcPosition - 2;
            if( maxLen > MAX_REF )
            {
                maxLen = MAX_REF;
            }

            if( literals == 0 )
            {
                // multiple back-references,
                // so there is no literal run control byte
                dstPosition--;
            }
            else
            {
                // set the control byte at the start of the literal run
                // to store the number of literals
                outBuff[ dstPosition - literals - 1 ] = static_cast<zp_uint8_t>( literals - 1 );
                literals = 0;
            }

            len = 3;
            while( len < maxLen && inBuff[ reff + len ] == inBuff[ srcPosition + len ] )
            {
                len++;
            }

            len -= 2;
            if( len < 7 )
            {
                outBuff[ dstPosition++ ] = static_cast<zp_uint8_t>( ( off >> 8 ) + ( len << 5 ) );
            }
            else
            {
                outBuff[ dstPosition++ ] = static_cast<zp_uint8_t>( ( off >> 8 ) + ( 7 << 5 ) );
                outBuff[ dstPosition++ ] = static_cast<zp_uint8_t>( len - 7 );
            }

            outBuff[ dstPosition++ ] = static_cast<zp_uint8_t>( off );
            // move one byte forward to allow for a literal run control byte
            dstPosition++;
            srcPosition += len;
            // Rebuild the future, and store the last bytes to the hashtable.
            // Storing hashes of the last bytes in back-reference improves
            // the compression ratio and only reduces speed slightly.
            future = zp_lzf_first( inBuff, srcPosition );
            future = zp_lzf_next( future, inBuff, srcPosition );
            hashTab[ zp_lzf_hash( future ) ] = srcPosition++;
            future = zp_lzf_next( future, inBuff, srcPosition );
            hashTab[ zp_lzf_hash( future ) ] = srcPosition++;
        }
        else
        {
            // copy one byte from input to output as part of literal
            outBuff[ dstPosition++ ] = inBuff[ srcPosition++ ];
            literals++;
            // at the end of this literal chunk, write the length
            // to the control byte and start a new chunk
            if( literals == MAX_LITERAL )
            {
                outBuff[ dstPosition - literals - 1 ] = static_cast<zp_uint8_t>( literals - 1 );
                literals = 0;
                // move ahead one byte to allow for the
                // literal run control byte
                dstPosition++;
            }
        }
    }

    // write the remaining few bytes as literals
    while( srcPosition < srcSize )
    {
        outBuff[ dstPosition++ ] = inBuff[ srcPosition++ ];
        literals++;
        if( literals == MAX_LITERAL )
        {
            outBuff[ dstPosition - literals - 1 ] = static_cast<zp_uint8_t>( literals - 1 );
            literals = 0;
            dstPosition++;
        }
    }

    // writes the final literal run length to the control byte
    outBuff[ dstPosition - literals - 1 ] = static_cast<zp_uint8_t>( literals - 1 );
    if( literals == 0 )
    {
        dstPosition--;
    }

    return dstPosition;
}

void zp_lzf_expand( const void* srcBuffer, zp_size_t srcPosition, zp_size_t srcSize, void* dstBuffer, zp_size_t dstPosition, zp_size_t dstSize )
{
    zp_size_t ctrl;
    zp_size_t len;
    zp_size_t i;

    const zp_uint8_t* inBuff = static_cast<const zp_uint8_t*>( srcBuffer );
    zp_uint8_t* outBuff = static_cast<zp_uint8_t*>( dstBuffer );

    do
    {
        ctrl = inBuff[ srcPosition++ ] & 0xFF;
        if( ctrl < MAX_LITERAL )
        {
            // literal run of length = ctrl + 1,
            ctrl++;
            // copy to output and move forward this many bytes
            for( i = 0; i != ctrl; i++ )
            {
                outBuff[ dstPosition++ ] = inBuff[ srcPosition++ ];
            }
            //System.Buffer.BlockCopy( inBuff, srcPosition, outBuff, dstPosition, ctrl );
            //System.Array.Copy( inBuff, srcPosition, outBuff, dstPosition, ctrl );
            //dstPosition += ctrl;
            //srcPosition += ctrl;
        }
        else
        {
            // back reference
            // the highest 3 bits are the match length
            len = ctrl >> 5;
            // if the length is maxed, add the next byte to the length
            if( len == 7 )
            {
                len += inBuff[ srcPosition++ ] & 0xFF;
            }
            // minimum back-reference is 3 bytes,
            // so 2 was subtracted before storing size
            len += 2;

            // ctrl is now the offset for a back-reference...
            // the logical AND operation removes the length bits
            ctrl = -( ( ctrl & 0x1f ) << 8 ) - 1;

            // the next byte augments/increases the offset
            ctrl -= inBuff[ srcPosition++ ] & 0xFF;

            // copy the back-reference bytes from the given
            // location in output to current position
            ctrl += dstPosition;
            if( dstPosition + len >= dstSize )
            {
                // reduce array localBounds checking
                break;
            }

            for( i = 0; i != len; i++ )
            {
                outBuff[ dstPosition++ ] = outBuff[ ctrl++ ];
            }
            //System.Buffer.BlockCopy( outBuff, ctrl, outBuff, dstPosition, len );
            //System.Array.Copy( outBuff, ctrl, outBuff, dstPosition, len );
            //dstPosition += len;
            //ctrl += len;
        }
    }
    while( dstPosition < dstSize );
}
