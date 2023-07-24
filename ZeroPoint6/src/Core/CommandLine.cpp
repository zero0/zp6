//
// Created by phosg on 7/16/2023.
//

#include "Core/CommandLine.h"

namespace zp
{
    CommandLine::CommandLine( MemoryLabel memoryLabel )
        : m_commandLineOperations( 16, memoryLabel )
        , m_commandLineTokens( 16, memoryLabel )
        , memoryLabel( memoryLabel )
    {
    }

    CommandLineOperation CommandLine::addOperation( const CommandLineOperationDesc& desc )
    {
        zp_size_t id = m_commandLineOperations.size() + 1; // NOTE: 0 is null handle;
        m_commandLineOperations.pushBack( desc );

        return { .id = id };
    }

    zp_bool_t CommandLine::parse( const char* cmdLine )
    {
        zp_bool_t ok = true;

        const zp_size_t len = zp_strlen( cmdLine );
        if( len > 0 )
        {
            zp_bool_t inQuote = false;
            zp_bool_t inToken = false;
            zp_size_t tokenStart = 0;
            for( zp_size_t i = 0; i < len; ++i )
            {
                const zp_bool_t wasInToken = inToken;

                const char c = cmdLine[ i ];

                if( inQuote )
                {
                    inToken = true;
                    switch( c )
                    {
                        case '"':
                            inQuote = false;
                            break;

                        case '\\':
                            ++i;
                            break;

                        default:
                            break;
                    }
                }
                else
                {
                    switch( c )
                    {
                        case ' ':
                        case '\t':
                            inToken = false;
                            break;

                        case '"':
                            inQuote = !inQuote;
                            if( inQuote )
                            {
                                inToken = true;
                            }
                            break;

                        case '\\':
                            break;

                        default:
                            inToken = true;
                            break;
                    }
                }


                if( wasInToken && !inToken )
                {
                    m_commandLineTokens.pushBack( {
                        .str = reinterpret_cast<const zp_char8_t*>(cmdLine + tokenStart),
                        .length = i - tokenStart
                    } );
                }
                else if( !wasInToken && inToken )
                {
                    tokenStart = i;
                }
            }

            if( inToken )
            {
                inToken = false;
                m_commandLineTokens.pushBack( {
                    .str = reinterpret_cast<const zp_char8_t*>(cmdLine + tokenStart),
                    .length= len - tokenStart
                } );
            }

            ok = !inQuote && !inToken;
        }

        return ok;
    }

    zp_bool_t CommandLine::hasFlag( const CommandLineOperation& operation, zp_bool_t includeParameters ) const
    {
        zp_bool_t found = false;

        if( operation.id )
        {
            const zp_size_t index = operation.id - 1;
            const CommandLineOperationDesc& desc = m_commandLineOperations[ index ];
            found = hasFlag( desc, includeParameters );
        }

        return found;
    }

    zp_bool_t CommandLine::hasParameter( const CommandLineOperation& operation, Vector<String>& outParameters ) const
    {
        zp_bool_t found = false;

        if( operation.id )
        {
            const zp_size_t index = operation.id - 1;
            const CommandLineOperationDesc& desc = m_commandLineOperations[ index ];
            found = hasParameter( desc, outParameters );
        }

        return found;
    }

    zp_bool_t CommandLine::hasFlag( const CommandLineOperationDesc& desc, zp_bool_t includeParameters ) const
    {
        zp_bool_t hasFlag = false;

        const zp_bool_t valid = includeParameters || ( desc.maxParameterCount == 0 && desc.minParameterCount == 0 );

        if( valid )
        {
            for( const String& token : m_commandLineTokens )
            {
                zp_int32_t cmp = zp_strcmp( token.str, token.length, desc.shortName.str, desc.shortName.length );
                if( cmp != 0 )
                {
                    cmp = zp_strcmp( token.str, token.length, desc.longName.str, desc.longName.length );
                }

                if( cmp == 0 )
                {
                    hasFlag = true;
                    break;
                }
            }
        }

        return hasFlag;
    }

    zp_bool_t CommandLine::hasParameter( const CommandLineOperationDesc& desc, Vector<String>& outParameters ) const
    {
        zp_bool_t found = false;

        const zp_bool_t valid = desc.maxParameterCount >= desc.minParameterCount;

        if( valid )
        {
            zp_size_t parameterIndex = 0;

            for( zp_size_t i = 0; i < m_commandLineTokens.size(); ++i )
            {
                const String& token = m_commandLineTokens[ i ];

                zp_int32_t cmp = zp_strcmp( token.str, token.length, desc.shortName.str, desc.shortName.length );
                if( cmp != 0 )
                {
                    cmp = zp_strcmp( token.str, token.length, desc.longName.str, desc.longName.length );
                }

                if( cmp == 0 )
                {
                    parameterIndex = i + 1;
                    found = true;
                    break;
                }
            }

            if( found )
            {
                outParameters.clear();

                zp_size_t i, idx;
                for( i = 0, idx = parameterIndex; i < desc.maxParameterCount && idx < m_commandLineTokens.size(); ++i, ++idx )
                {
                    const String& token = m_commandLineTokens[ idx ];
                    if( token[ 0 ] == '-' )
                    {
                        break;
                    }

                    outParameters.pushBack( token );
                }

                if( i < desc.minParameterCount )
                {
                    found = false;
                }
            }
        }

        return found;
    }

    void CommandLine::printHelp() const
    {
        for( const String& t : m_commandLineTokens )
        {
            zp_printfln( "= %.*s", t.length, t.c_str() );
        }

        for( const auto& op : m_commandLineOperations )
        {
            if( !op.longName.empty() && op.shortName.empty() )
            {
                zp_printfln( "%-20.*s%.*s", op.longName.length, op.longName.str, op.description.length, op.description.str );
            }
            else if( op.longName.empty() && !op.shortName.empty() )
            {
                zp_printfln( "%-20.*s%.*s", op.shortName.length, op.shortName.str, op.description.length, op.description.str );
            }
            else if( !op.longName.empty() && !op.shortName.empty() )
            {
                zp_printfln( "%-5.*s,%-14.*s%.*s", op.shortName.length, op.shortName.str, op.longName.length, op.longName.str, op.description.length, op.description.str );
            }
        }
    }
}