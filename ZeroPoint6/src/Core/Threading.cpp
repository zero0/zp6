#include "Core/Threading.h"
#include "Platform/Platform.h"

namespace zp
{
    MutexAutoScope::MutexAutoScope( zp_handle_t mutex, zp_time_t millisecondTimeout )
        : m_mutex( Platform::AcquireMutex( mutex, millisecondTimeout ) == Platform::AcquireMutexResult::Acquired ? mutex : nullptr )
    {
    }

    MutexAutoScope::~MutexAutoScope()
    {
        if( m_mutex )
        {
            Platform::ReleaseMutex( m_mutex );
            m_mutex = nullptr;
        }
    }

    zp_bool_t MutexAutoScope::acquired() const
    {
        return m_mutex;
    }
}
