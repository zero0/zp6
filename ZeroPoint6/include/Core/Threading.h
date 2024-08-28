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

#endif //ZP_THREADING_H
