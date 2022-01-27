//
// Created by phosg on 11/10/2021.
//

#include "Engine/Engine.h"

namespace zp
{
    Engine::Engine( MemoryLabel memoryLabel )
        : memoryLabel( memoryLabel )
        , m_exitCode( 0 )
        , m_isRunning( false )
    {
    }

    Engine::~Engine()
    {

    }

    zp_bool_t Engine::isRunning() const
    {
        return m_isRunning;
    }

    zp_int32_t Engine::getExitCode() const
    {
        return m_exitCode;
    }

    void Engine::initialize()
    {

    }

    void Engine::destroy()
    {

    }

    void Engine::startEngine()
    {

    }

    void Engine::stopEngine( const zp_int32_t exitCode )
    {
        m_exitCode = exitCode;
        m_isRunning = false;
    }
}