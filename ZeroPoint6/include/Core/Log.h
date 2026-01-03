//
// Created by phosg on 9/2/2024.
//

#ifndef ZP_LOG_H
#define ZP_LOG_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/String.h"

#define ZP_USE_LOG_MESSAGE      ( ZP_DEBUG_BUILD )
#define ZP_USE_LOG_INFO         ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_PERFORMANCE  ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_WARNING      ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_ERROR        ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_FATAL        ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD || ZP_DISTRIBUTION_BUILD )

// @formatter:off
#define ZP_LOG_MSG(x)       do { zp::Log::message()  << __FILE__ << ":" << __LINE__ << " " << x << zp::Log::endl; } while(0)
#define ZP_LOG_INFO(x)      do { zp::Log::info()     << __FILE__ << ":" << __LINE__ << " " << x << zp::Log::endl; } while(0)
#define ZP_LOG_PERF(x)      do { zp::Log::perf()     << __FILE__ << ":" << __LINE__ << " " << x << zp::Log::endl; } while(0)
#define ZP_LOG_WARN(x)      do { zp::Log::warning()  << __FILE__ << ":" << __LINE__ << " " << x << zp::Log::endl; } while(0)
#define ZP_LOG_ERROR(x)     do { zp::Log::error()    << __FILE__ << ":" << __LINE__ << " " << x << zp::Log::endl; } while(0)
#define ZP_LOG_FATAL(x)     do { zp::Log::fatal()    << __FILE__ << ":" << __LINE__ << " " << x << zp::Log::endl; } while(0)
// @formatter:on

namespace zp
{
    enum class LogType
    {
        Null,
        Message,
        Info,
        Performance,
        Warning,
        Error,
        Fatal,
        Count,
    };

    // @formatter:off
    constexpr LogType LogMessageType =      ZP_USE_LOG_MESSAGE ?        LogType::Message : LogType::Null;
    constexpr LogType LogInfoType =         ZP_USE_LOG_INFO ?           LogType::Info : LogType::Null;
    constexpr LogType LogPerformanceType =  ZP_USE_LOG_PERFORMANCE ?    LogType::Performance : LogType::Null;
    constexpr LogType LogWarningType =      ZP_USE_LOG_WARNING ?        LogType::Warning : LogType::Null;
    constexpr LogType LogErrorType =        ZP_USE_LOG_ERROR ?          LogType::Error : LogType::Null;
    constexpr LogType LogFatalType =        ZP_USE_LOG_FATAL ?          LogType::Fatal : LogType::Null;
    // @formatter:on

    template<LogType Type>
    struct LogEntry;

    // @formatter:off
    using LogEntryMessage =     LogEntry<LogMessageType>;
    using LogEntryInfo =        LogEntry<LogInfoType>;
    using LogEntryPerformance = LogEntry<LogPerformanceType>;
    using LogEntryWarning =     LogEntry<LogWarningType>;
    using LogEntryError =       LogEntry<LogErrorType>;
    using LogEntryFatal =       LogEntry<LogFatalType>;
    // @formatter:on


    namespace Log
    {
        enum EndToken
        {
            endl
        };

        auto message() -> LogEntryMessage;

        auto info() -> LogEntryInfo;

        auto perf() -> LogEntryPerformance;

        auto warning() -> LogEntryWarning;

        auto error() -> LogEntryError;

        auto fatal() -> LogEntryFatal;
    }

    template<LogType Type>
    struct LogEntry
    {
    public:
        using value_type = LogEntry<Type>;
        using reference_type = value_type&;

        auto operator<<( const char* str ) -> reference_type;

        auto operator<<( const String& str ) -> reference_type;

        auto operator<<( zp_int32_t value ) -> reference_type;

        auto operator<<( zp_uint32_t value ) -> reference_type;

        auto operator<<( zp_int64_t value ) -> reference_type;

        auto operator<<( zp_uint64_t value ) -> reference_type;

        auto operator<<( zp_float32_t value ) -> reference_type;

        auto operator<<( const void* ptr ) -> reference_type;

        void operator<<( Log::EndToken );

    private:
        enum
        {
            kLogEntryLineSize = 1 KB
        };

        MutableFixedString<kLogEntryLineSize> m_log;
    };

    template<LogType Type>
    auto LogEntry<Type>::operator<<( const char* str ) -> reference_type
    {
        if( str != nullptr )
        {
            m_log.append( str );
        }
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( const String& str ) -> reference_type
    {
        if( !str.empty() )
        {
            m_log.append( str.c_str(), str.length() );
        }
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( zp_int32_t value ) -> reference_type
    {
        m_log.appendFormat( "%d", value );
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( zp_uint32_t value ) -> reference_type
    {
        m_log.appendFormat( "%u", value );
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( zp_int64_t value ) -> reference_type
    {
        m_log.appendFormat( "%ld", value );
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( zp_uint64_t value ) -> reference_type
    {
        m_log.appendFormat( "%lu", value );
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( zp_float32_t value ) -> reference_type
    {
        m_log.appendFormat( "%f", value );
        return *this;
    }

    template<LogType Type>
    auto LogEntry<Type>::operator<<( const void* ptr ) -> reference_type
    {
        m_log.appendFormat( "%p", ptr );
        return *this;
    }

    //
    // Null LogEntry
    //

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( const char* ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( const String& ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( zp_int32_t ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( zp_uint32_t ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( zp_int64_t ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( zp_uint64_t ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( zp_float32_t ) -> reference_type
    {
        return *this;
    }

    template<>
    inline auto LogEntry<LogType::Null>::operator<<( const void* ) -> reference_type
    {
        return *this;
    }

    template<>
    inline void LogEntry<LogType::Null>::operator<<( Log::EndToken )
    {
    }
}

#endif //ZP_LOG_H
