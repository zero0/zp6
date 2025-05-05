//
// Created by phosg on 3/25/2025.
//

#include "Core/Types.h"
#include "Core/Data.h"
#include "Core/Memory.h"
#include "Core/String.h"
#include "Core/Http.h"

#define CR  '\r'
#define LF  '\n'
#define CRLF "\r\n"

#define _CC(x)  x
#define CCCCCC( a, b,c )       _CC(a ## :: ## b ## :: ## c)
#define CCC( a, b )       _CC(a ## :: ## b)

namespace zp
{
    namespace Http
    {
        namespace
        {
            Http::Method ParseMethod( const String& string )
            {
#define TEST_METHOD(name, str)  else if( zp_strcmp( string, #name ) == 0 ) { method = ZP_HTTP_METHOD_##name; }
                Http::Method method = ZP_HTTP_METHOD_UNKNOWN;

                if( string.empty() )
                {
                }
                HTTP_METHODS( TEST_METHOD )

                return method;
#undef TEST_METHOD
            }

            Http::Version ParseVersion( const String& string )
            {
                Http::Version version = Http::Version::Unknown;

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
#define TEST_HEADER(name, str)  else if( zp_strcmp( string, #str ) == 0 ) { key = ZP_HTTP_HEADER_##name; }

                Http::HeaderKey key = ZP_HTTP_HEADER_UNKNOWN;

                if( string.empty() )
                {
                }
                HTTP_HEADERS( TEST_HEADER )

                return key;

#undef TEST_HEADER
            }

            void WriteHeaderKey( const HeaderKey headerKey, DataStreamWriter& writer )
            {
                switch( headerKey )
                {
#define WRITE_HEADER(name, str)     case ZP_HTTP_HEADER_##name: writer.write( #str ); break;
                    HTTP_HEADERS( WRITE_HEADER )
#undef WRITE_HEADER
                    default:
                        ZP_INVALID_CODE_PATH();
                        break;
                }
            }

            void WriteHeader( const Header& header, DataStreamWriter& writer )
            {
                if( header.key != ZP_HTTP_HEADER_UNKNOWN )
                {
                    WriteHeaderKey( header.key, writer );
                    writer.write( ':' );
                    writer.write( ' ' );
                    writer.write( header.value.c_str(), header.value.length() );
                    writer.write( CRLF );
                }
            }

            void WriteStatusCode( const StatusCode statusCode, DataStreamWriter& writer )
            {
                switch( statusCode )
                {
#define WRITE_STATUS(code, name, str)   case ZP_HTTP_STATUS_##name: writer.write( #code " " #str ); break;
                    HTTP_STATUS_CODES( WRITE_STATUS )
#undef WRITE_STATUS
                    default:
                        break;
                }
            }

            void WriteVersion( Version version, DataStreamWriter& writer )
            {
                writer.write( "HTTP/1.1" );
                writer.write( "" );
            }

            void WriteResponseHeader( const Response& response, DataStreamWriter& writer )
            {
                WriteVersion( response.version, writer );
                writer.write( ' ' );
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

                const String string( inMemory.as<zp_char8_t>(), inMemory.size );
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
                                    .value = headerTokenizer.remaining(),
                                    .key = ParseHeaderKey( key ),
                                };

                                if( header.key != ZP_HTTP_HEADER_UNKNOWN )
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
                            break;
                    }
                }

                outRequest.statusCode = ZP_HTTP_STATUS_OK;
            }
        }
    }

    Http::StatusCode Http::ParseRequest( const Memory& inMemory, Http::Request& outRequest )
    {
        ParseRequestImpl( inMemory, outRequest );
        return outRequest.statusCode;
    }

    Http::StatusCode Http::WriteResponse( const Http::Response& inResponse, DataStreamWriter& outMemory )
    {
        WriteResponseHeader( inResponse, outMemory );

        for( const auto& header : inResponse.headers )
        {
            WriteHeader( header, outMemory );
        }

        outMemory.write( CRLF );

        return Http::ZP_HTTP_STATUS_OK;
    }
}
