//
// Created by phosg on 1/2/2026.
//

#ifndef ZP_PROPERTIES_H
#define ZP_PROPERTIES_H

#include "Core/Types.h"
#include "Core/String.h"
#include "Core/Map.h"

namespace zp
{
    class Properties
    {
    public:
        Properties( MemoryLabel memoryLabel );

        zp_bool_t TryParse( const char* properties );

        zp_bool_t TryParse( const String& properties );

        zp_bool_t TryGetProperty( const String& propertyName, String& propertyValue );

        zp_bool_t TryGetPropertyAsBoolean( const String& propertyName, zp_bool_t& propertyValue );

        zp_bool_t TryGetPropertyAsInt32( const String& propertyName, zp_int32_t& propertyValue );

        zp_bool_t TryGetPropertyAsSizeT( const String& propertyName, zp_size_t& propertyValue );

    private:
        String m_propertyString;
        Map<String, String> m_properties;

    public:
        MemoryLabel memoryLabel;
    };
}

#endif //ZP_PROPERTIES_H
