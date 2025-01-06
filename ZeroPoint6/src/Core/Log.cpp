//
// Created by phosg on 9/2/2024.
//

#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Log.h"
#include "Core/Common.h"
#include "Core/String.h"

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
            "[MSG]",
            "[INFO]",
            "[WARN]",
            "[ERROR]",
            "[FATAL]"
        };
        ZP_STATIC_ASSERT( ZP_ARRAY_SIZE( LogTypeNames ) == (int)LogType::Count );

        template<zp_bool_t IsError, LogType LogType>
        void PrintLogEntry( const char* color, const char* msg )
        {
            const DateTime dateTime = Platform::DateTimeNowLocal();

            const zp_size_t kDateTimeStrSize = 64;
            MutableFixedString<kDateTimeStrSize> dateTimeStr;
            Platform::DateTimeToString( dateTime, dateTimeStr.mutable_str(), dateTimeStr.capacity(), "%Y-%m-%d %H:%M:%S." );

            const zp_uint32_t threadID = Platform::GetCurrentThreadId();

            const char* format = "%s%23s %7s (%10d): %s " ZP_CC_RESET;

            if constexpr( IsError )
            {
                zp_error_printfln( format, color, dateTimeStr.c_str(), LogTypeNames[ (int)LogType ], threadID, msg );
            }
            else
            {
                zp_printfln( format, color, dateTimeStr.c_str(), LogTypeNames[ (int)LogType ], threadID, msg );
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
        PrintLogEntry<false, LogType::Message>( ZP_CC( NORMAL, BLUE, DEFAULT ), m_log.c_str() );
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
        PrintLogEntry<false, LogType::Info>( ZP_CC( NORMAL, DEFAULT, DEFAULT ), m_log.c_str() );
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
        PrintLogEntry<false, LogType::Warning>( ZP_CC( NORMAL, YELLOW, DEFAULT ), m_log.c_str() );
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
        PrintLogEntry<true, LogType::Error>( ZP_CC( NORMAL, RED, DEFAULT ), m_log.c_str() );
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
        PrintLogEntry<true, LogType::Fatal>( ZP_CC( BOLD, RED, DEFAULT ), m_log.c_str() );
    }
}
