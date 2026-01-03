//
// Created by phosg on 1/2/2026.
//

#include "Core/Types.h"
#include "Core/String.h"
#include "Core/Properties.h"

namespace zp
{
    Properties::Properties( MemoryLabel memoryLabel )
        : m_propertyString( {} )
        , m_properties( memoryLabel )
        , memoryLabel( memoryLabel )
    {

    }

    zp_bool_t Properties::TryParse( const String& properties )
    {
        m_propertyString = properties;
        m_properties.clear();

        zp_bool_t parsed = true;

        Tokenizer newLineTokenizer( properties, "\n\r" );

        String lineToken;
        while( newLineTokenizer.next( lineToken ) )
        {
            if( lineToken.empty() )
            {
                continue;
            }

            if( lineToken[ 0 ] == '#' )
            {
                continue;
            }

            String propertyName;
            String propertyValue;

            {

                Tokenizer propertyTokenizer( lineToken, "=" );

                zp_bool_t ok = propertyTokenizer.next( propertyName );
                if( !ok )
                {
                    parsed = false;
                    break;
                }

                propertyValue = propertyTokenizer.remaining();
            }

            m_properties[ propertyName ] = propertyValue;
        }

        return parsed;
    }

    zp_bool_t Properties::TryGetProperty( const String& propertyName, String& propertyValue )
    {
        return m_properties.tryGet( propertyName, propertyValue );
    }

    zp_bool_t Properties::TryGetPropertyAsBoolean( const String& propertyName, zp_bool_t& propertyValue )
    {
        String propertyStrValue;

        const zp_bool_t found = TryGetProperty( propertyName, propertyStrValue );
        if( found )
        {
            if( !propertyStrValue.empty() )
            {
                const zp_char8_t c = propertyStrValue[ 0 ];
                switch( c )
                {
                    case '1':
                        propertyValue = propertyStrValue.length() == 1;
                        break;
                    case 't':
                        propertyValue = zp_strcmp( propertyStrValue, "true" ) == 0;
                        break;
                    case 'T':
                        propertyValue = zp_strcmp( propertyStrValue, "TRUE" ) == 0;
                        break;
                    default:
                        propertyValue = false;
                        break;
                }
            }
        }

        return found;
    }

    zp_bool_t Properties::TryGetPropertyAsInt32( const String& propertyName, zp_int32_t& propertyValue )
    {
        String propertyStrValue;

        const zp_bool_t found = TryGetProperty( propertyName, propertyStrValue );
        if( found )
        {
            propertyValue = zp_atoi32( propertyStrValue.c_str() );
        }

        return found;
    }

    zp_bool_t Properties::TryGetPropertyAsSizeT( const String& propertyName, zp_size_t& propertyValue )
    {
        String propertyStrValue;

        const zp_bool_t found = TryGetProperty( propertyName, propertyStrValue );
        if( found )
        {
            propertyValue = static_cast<zp_size_t>(zp_atoi64( propertyStrValue.c_str() ));
        }

        return found;
    }
}
