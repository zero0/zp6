//
// Created by phosg on 9/2/2024.
//

#ifndef ZP_LOG_H
#define ZP_LOG_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/String.h"

#define ZP_USE_LOG_MESSAGE  ( ZP_DEBUG_BUILD )
#define ZP_USE_LOG_INFO     ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_WARNING  ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_ERROR    ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD )
#define ZP_USE_LOG_FATAL    ( ZP_DEBUG_BUILD || ZP_RELEASE_BUILD || ZP_DISTRIBUTION_BUILD )

namespace zp
{
    enum class LogType
    {
        Null,
        Message,
        Info,
        Warning,
        Error,
        Fatal,
        Count,
    };

    // @formatter:off
    constexpr LogType LogMessageType =  ZP_USE_LOG_MESSAGE ?    LogType::Message : LogType::Null;
    constexpr LogType LogInfoType =     ZP_USE_LOG_INFO ?       LogType::Info : LogType::Null;
    constexpr LogType LogWarningType =  ZP_USE_LOG_WARNING ?    LogType::Warning : LogType::Null;
    constexpr LogType LogErrorType =    ZP_USE_LOG_ERROR ?      LogType::Error : LogType::Null;
    constexpr LogType LogFatalType =    ZP_USE_LOG_FATAL ?      LogType::Fatal : LogType::Null;
    // @formatter:on

    template<LogType Type>
    struct LogEntry;

    // @formatter:off
    using LogEntryMessage = LogEntry<LogMessageType>;
    using LogEntryInfo =    LogEntry<LogInfoType>;
    using LogEntryWarning = LogEntry<LogWarningType>;
    using LogEntryError =   LogEntry<LogErrorType>;
    using LogEntryFatal =   LogEntry<LogFatalType>;
    // @formatter:on

    class Log
    {
    public:
        enum EndToken
        {
            endl
        };

        static LogEntryMessage message();

        static LogEntryInfo info();

        static LogEntryWarning warning();

        static LogEntryError error();

        static LogEntryFatal fatal();
    };

    template<LogType Type>
    class LogEntry
    {
    public:
        LogEntry& operator<<( const char* str );

        LogEntry& operator<<( zp_int32_t value );

        void operator<<( Log::EndToken );

    private:
        enum
        {
            kLogEntryLineSize = 256
        };

        MutableFixedString<kLogEntryLineSize> m_log;
    };
}

#endif //ZP_LOG_H
