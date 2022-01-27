//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_ENGINE_H
#define ZP_ENGINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"

namespace zp
{
    class Engine
    {
    ZP_NONCOPYABLE( Engine )

    public:
        explicit Engine( MemoryLabel memoryLabel );

        ~Engine();

        zp_bool_t isRunning() const;

        zp_int32_t getExitCode() const;

        void initialize();

        void destroy();

        void startEngine();

        void stopEngine( zp_int32_t exitCode );

    public:
        const MemoryLabel memoryLabel;

    private:
        zp_int32_t m_exitCode;
        zp_bool_t m_isRunning;
    };
}

#endif //ZP_ENGINE_H
