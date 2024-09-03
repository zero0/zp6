//
// Created by phosg on 9/2/2024.
//

#include "Core/Log.h"
#include "Core/Macros.h"

#include "Platform/Platform.h"

namespace zp
{
    LogEntryMessage Log::message()
    {
        return {};
    }

    LogEntryInfo Log::info()
    {
        return {};
    }

    LogEntryWarning Log::warning()
    {
        return {};
    }

    LogEntryError Log::error()
    {
        return {};
    }

    LogEntryFatal Log::fatal()
    {
        return {};
    }

    namespace
    {
        const char* LogTypeNames[] {
            "",
            "  [MSG]",
            " [INFO]",
            " [WARN]",
            "[ERROR]",
            "[FATAL]"
        };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( LogTypeNames ) == (int)LogType::Count );

        template<zp_bool_t IsError>
        void PrintLogEntry( const char* color, LogType logType, const char* msg )
        {
            const DateTime dateTime = Platform::DateTimeNowLocal();

            char dt[64];
            Platform::DateTimeToString( dateTime, dt,  "%Y-%m-%d %H:%M:%S." );

            if( IsError )
            {
                zp_error_printfln( "%s%s %s: %s" ZP_CC_RESET, color, dt, LogTypeNames[ (int)logType ], msg );
            }
            else
            {
                zp_printfln( "%s%s %s: %s" ZP_CC_RESET, color, dt, LogTypeNames[ (int)logType ], msg );
            }
        }
    }
}

namespace zp
{
    template<>
    LogEntry<LogType::Null>& LogEntry<LogType::Null>::operator<<( const char* str )
    {
        return *this;
    }

    template<>
    LogEntry<LogType::Null>& LogEntry<LogType::Null>::operator<<( zp_int32_t value )
    {
        return *this;
    }

    template<>
    void LogEntry<LogType::Null>::operator<<( Log::EndToken )
    {
    }
}

namespace zp
{
    template<>
    LogEntry<LogType::Message>& LogEntry<LogType::Message>::operator<<( const char* str )
    {
        m_log.append( str );
        return *this;
    }

    template<>
    LogEntry<LogType::Message>& LogEntry<LogType::Message>::operator<<( zp_int32_t value )
    {
        m_log.appendFormat( "%d", value );
        return *this;
    }

    template<>
    void LogEntry<LogType::Message>::operator<<( Log::EndToken )
    {
        PrintLogEntry<false>( ZP_CC( NORMAL, BLUE, DEFAULT ), LogType::Message, m_log.c_str() );
    }
}

namespace zp
{
    template<>
    LogEntry<LogType::Info>& LogEntry<LogType::Info>::operator<<( const char* str )
    {
        m_log.append( str );
        return *this;
    }

    template<>
    LogEntry<LogType::Info>& LogEntry<LogType::Info>::operator<<( zp_int32_t value )
    {
        m_log.appendFormat( "%d", value );
        return *this;
    }

    template<>
    void LogEntry<LogType::Info>::operator<<( Log::EndToken )
    {
        PrintLogEntry<false>( ZP_CC( NORMAL, DEFAULT, DEFAULT ), LogType::Info, m_log.c_str() );
    }
}

namespace zp
{
    template<>
    LogEntry<LogType::Warning>& LogEntry<LogType::Warning>::operator<<( const char* str )
    {
        m_log.append( str );
        return *this;
    }

    template<>
    LogEntry<LogType::Warning>& LogEntry<LogType::Warning>::operator<<( zp_int32_t value )
    {
        m_log.appendFormat( "%d", value );
        return *this;
    }

    template<>
    void LogEntry<LogType::Warning>::operator<<( Log::EndToken )
    {
        PrintLogEntry<false>( ZP_CC( NORMAL, YELLOW, DEFAULT ), LogType::Warning, m_log.c_str() );
    }
}

namespace zp
{
    template<>
    LogEntry<LogType::Error>& LogEntry<LogType::Error>::operator<<( const char* str )
    {
        m_log.append( str );
        return *this;
    }

    template<>
    LogEntry<LogType::Error>& LogEntry<LogType::Error>::operator<<( zp_int32_t value )
    {
        m_log.appendFormat( "%d", value );
        return *this;
    }

    template<>
    void LogEntry<LogType::Error>::operator<<( Log::EndToken )
    {
        PrintLogEntry<true>( ZP_CC( NORMAL, RED, DEFAULT ), LogType::Error, m_log.c_str() );
    }
}

namespace zp
{
    template<>
    LogEntry<LogType::Fatal>& LogEntry<LogType::Fatal>::operator<<( const char* str )
    {
        m_log.append( str );
        return *this;
    }

    template<>
    LogEntry<LogType::Fatal>& LogEntry<LogType::Fatal>::operator<<( zp_int32_t value )
    {
        m_log.appendFormat( "%d", value );
        return *this;
    }

    template<>
    void LogEntry<LogType::Fatal>::operator<<( Log::EndToken )
    {
        PrintLogEntry<true>( ZP_CC( BOLD, RED, DEFAULT ), LogType::Fatal, m_log.c_str() );
    }
}
