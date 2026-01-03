//
// Created by phosg on 3/25/2025.
//

#include "Core/Types.h"
#include "Core/Data.h"
#include "Core/Memory.h"
#include "Core/String.h"
#include "Core/Http.h"

#include "Platform/Platform.h"

#define CR  '\r'
#define LF  '\n'
#define CRLF "\r\n"

namespace zp
{
    namespace Http
    {
        namespace
        {
            Http::Method ParseMethod( const String& string )
            {
#define TEST_METHOD(name, str)  else if( zp_strcmp( string, #name ) == 0 ) { method = Method::name; }
                Http::Method method = Method::UNKNOWN;

                if( string.empty() )
                {
                }
                HTTP_METHODS( TEST_METHOD )

                return method;
#undef TEST_METHOD
            }

            Http::Version ParseVersion( const String& string )
            {
                Http::Version version = Http::Version::UNKNOWN;

                const char* httpHeader = zp_strnstr( string, "HTTP/" );
                if( string.length() >= zp_strlen( "HTTP/x.x" ) && httpHeader != nullptr )
                {
                    const char major = httpHeader[ 5 ];
                    const char minor = httpHeader[ 7 ];
                    switch( major )
                    {
                        case '1':
                        {
                            switch( minor )
                            {
                                case '0':
                                    version = Http::Version::Http1_0;
                                    break;
                                case '1':
                                    version = Http::Version::Http1_1;
                                    break;
                                default:
                                    ZP_INVALID_CODE_PATH();
                                    break;
                            }
                        }
                        break;

                        case '2':
                        {
                            switch( minor )
                            {
                                case '0':
                                    version = Http::Version::Http2_0;
                                    break;
                                default:
                                    ZP_INVALID_CODE_PATH();
                                    break;
                            }
                        }
                        break;

                        default:
                            ZP_INVALID_CODE_PATH();
                            break;
                    }
                }

                return version;
            }

            Http::HeaderKey ParseHeaderKey( const String& string )
            {
#define TEST_HEADER(name, str, type)  else if( zp_strcmp( string, #str ) == 0 ) { key = HeaderKey::name; }

                Http::HeaderKey key = HeaderKey::UNKNOWN;

                if( string.empty() )
                {
                }
                HTTP_HEADERS( TEST_HEADER )

                return key;

#undef TEST_HEADER
            }

            void WriteHeaderKey( const Header& header, DataStreamWriter& writer )
            {
                switch( header.key )
                {
#define WRITE_HEADER_KEY(name, str, type)     case HeaderKey::name: writer.write( #str ); break;
                    HTTP_HEADERS( WRITE_HEADER_KEY )
#undef WRITE_HEADER_KEY
                    default:
                        ZP_INVALID_CODE_PATH();
                        break;
                }
            }

            void WriteHeaderValue( const Header& header, DataStreamWriter& writer )
            {
                MutableFixedString64 buffer;
                switch( header.key )
                {
#define WRITE_HEADER_VALUE(name, str, type)     \
    case HeaderKey::name: \
        if constexpr( type == HeaderValueType::String )\
        {\
            writer.write( header.stringValue.c_str(), header.stringValue.length() ); \
        }\
        else if constexpr( type == HeaderValueType::Int )\
        {\
            buffer.format( "%lu", header.intValue );\
            writer.write( buffer.c_str(), buffer.length() ); \
        }\
        else if constexpr( type == HeaderValueType::Date )\
        {\
            const DateTime dt {}; \
            const zp_size_t len = Platform::DateTimeToString( dt, buffer.mutable_str(), buffer.length() ); \
            writer.write( header.stringValue.c_str(), len ); \
        }\
        break;
                    HTTP_HEADERS( WRITE_HEADER_VALUE )
#undef WRITE_HEADER_VALUE
                    default:
                        ZP_INVALID_CODE_PATH();
                        break;
                }
            }

            void WriteHeaderEntry( const Header& header, DataStreamWriter& writer )
            {
                if( header.key != HeaderKey::UNKNOWN )
                {
                    WriteHeaderKey( header, writer );
                    writer.write( ": " );
                    WriteHeaderValue( header, writer );
                    writer.write( CRLF );
                }
            }

            void WriteStatusCode( const StatusCode statusCode, DataStreamWriter& writer )
            {
                switch( statusCode )
                {
#define WRITE_STATUS(code, name, str)   case StatusCode::name: writer.write( #code " " #str ); break;
                    HTTP_STATUS_CODES( WRITE_STATUS )
#undef WRITE_STATUS
                    default:
                        break;
                }
            }

            void WriteVersion( Version version, DataStreamWriter& writer )
            {
                switch( version )
                {
                    case Version::Http1_0:
                        writer.write( "HTTP/1.0" );
                        break;
                    case Version::Http1_1:
                        writer.write( "HTTP/1.1" );
                        break;
                    case Version::Http2_0:
                        writer.write( "HTTP/2.0" );
                        break;
                }
            }

            void WriteResponseHeader( const Response& response, DataStreamWriter& writer )
            {
                WriteVersion( response.version, writer );
                writer.write( " " );
                WriteStatusCode( response.statusCode, writer );
                writer.write( CRLF );
            }

            void ParseRequestImpl( const Memory& inMemory, Http::Request& outRequest )
            {
                enum class ParseState
                {
                    Method,
                    Header,
                    Body,
                    End,
                };

                ParseState state = ParseState::Method;

                outRequest = {};

                const String string( inMemory.as<zp_char8_t>(), inMemory.size() );
                Tokenizer tokenizer( string, CRLF );

                String token {};
                while( state != ParseState::End && tokenizer.next( token ) )
                {
                    switch( state )
                    {
                        case ParseState::Method:
                        {
                            String methodToken {};
                            Tokenizer methodTokenizer( token, " " );

                            // method
                            methodTokenizer.next( methodToken );
                            outRequest.method = ParseMethod( methodToken );

                            // path
                            methodTokenizer.next( methodToken );
                            outRequest.path = methodToken;

                            // version
                            methodTokenizer.next( methodToken );
                            outRequest.version = ParseVersion( methodToken );

                            state = ParseState::Header;
                        }
                        break;

                        case ParseState::Header:
                        {
                            if( token.empty() )
                            {
                                state = ParseState::Body;
                            }
                            else
                            {
                                Tokenizer headerTokenizer( token, ":" );

                                String key {};
                                headerTokenizer.next( key );

                                const Header header {
                                    .stringValue = headerTokenizer.remaining(),
                                    .key = ParseHeaderKey( key ),
                                };

                                if( header.key != HeaderKey::UNKNOWN )
                                {
                                    outRequest.headers.pushBack( header );
                                }
                            }
                        }
                        break;

                        case ParseState::Body:
                        {
                            outRequest.body = tokenizer.remaining();

                            state = ParseState::End;
                        }
                        break;

                        default:
                            ZP_INVALID_CODE_PATH();
                            break;
                    }
                }

                outRequest.statusCode = StatusCode::OK;
            }
        }
    }

    Http::StatusCode Http::ParseRequest( const Memory& inMemory, Request& outRequest )
    {
        ParseRequestImpl( inMemory, outRequest );
        return outRequest.statusCode;
    }

    Http::StatusCode Http::WriteResponse( const Response& inResponse, DataStreamWriter& outMemory )
    {
        WriteResponseHeader( inResponse, outMemory );

        for( const auto& header : inResponse.headers )
        {
            WriteHeaderEntry( header, outMemory );
        }

        if( !inResponse.body.empty() )
        {
            WriteHeaderEntry( {
                .intValue = inResponse.body.length(),
                .key = HeaderKey::CONTENT_LENGTH,
            }, outMemory );

            outMemory.write( CRLF );

            outMemory.write( inResponse.body.data(), inResponse.body.size() );
        }

        return StatusCode::OK;
    }
}
