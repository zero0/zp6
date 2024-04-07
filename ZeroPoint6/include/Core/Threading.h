#ifndef ZP_THREADING_H
#define ZP_THREADING_H

#include "Core/Macros.h"
#include "Core/Types.h"

//
//
//

namespace zp
{
    class CriticalSection
    {
    public:
        CriticalSection();

        ~CriticalSection();

        CriticalSection( const CriticalSection& other );

        CriticalSection( CriticalSection&& other ) noexcept;

        void enter();

        void leave();

    private:
        zp_uint8_t m_memory[40]; // matches Windows size but can be larger
    };
}

//
//
//

namespace zp
{
    class MutexAutoScope
    {
    ZP_NONCOPYABLE( MutexAutoScope );

    public:
        MutexAutoScope() = delete;

        ZP_FORCEINLINE explicit MutexAutoScope( zp_handle_t mutex, zp_time_t millisecondTimeout = ZP_TIME_INFINITE );

        ZP_FORCEINLINE ~MutexAutoScope();

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t acquired() const;

    private:
        zp_handle_t m_mutex;
    };
}

#endif //ZP_THREADING_H
