//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_ENGINE_H
#define ZP_ENGINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"

#if ZP_USE_PROFILER

#include "Core/Profiler.h"

#endif

#include "Engine/JobSystem.h"
#include "Engine/ModuleEntryPointAPI.h"
#include "Engine/EnginePipeline.h"
#include "Engine/Entity.h"
#include "Engine/Component.h"

#include "Rendering/RenderSystem.h"

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

        void stopEngine();

        void restart();

        void exit( zp_int32_t exitCode );

        void process();

        void advanceFrame();

        zp_uint64_t getFrameCount() const
        {
            return m_frameCount;
        }

        ModuleEntryPointAPI* getModuleAPI() const
        {
            return m_moduleAPI;
        }

        JobSystem* getJobSystem() const
        {
            return m_jobSystem;
        }

        RenderSystem* getRenderSystem() const
        {
            return m_renderSystem;
        }

        EntityComponentManager* getEntityComponentManager() const
        {
            return m_entityComponentManager;
        }

    private:
        enum EngineState : zp_uint8_t
        {
            Idle,
            Initializing,
            Running,
            Restart,
            Exit,
        };

        static void onWindowResize( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height )
        {
            zp_printfln( "new w %d h %d", width, height );
        }

        zp_handle_t m_windowHandle;

        zp_handle_t m_moduleDll;
        ModuleEntryPointAPI* m_moduleAPI;

        WindowCallbacks m_windowCallbacks;

        EnginePipeline* m_currentEnginePipeline;
        EnginePipeline* m_nextEnginePipeline;

        JobHandle m_previousFrameEnginePipelineHandle;

        JobSystem* m_jobSystem;
        RenderSystem* m_renderSystem;
        EntityComponentManager* m_entityComponentManager;

#if ZP_USE_PROFILER
        Profiler* m_profiler;
#endif

        zp_size_t m_frameCount;

        zp_int32_t m_exitCode;
        EngineState m_nextEngineState;
        EngineState m_currentEngineState;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_ENGINE_H
