//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_ENGINE_H
#define ZP_ENGINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/Job.h"

#if ZP_USE_PROFILER

#include "Core/Profiler.h"

#endif

#include "Engine/ModuleEntryPointAPI.h"
#include "Engine/EnginePipeline.h"
#include "Engine/Entity.h"
#include "Engine/Component.h"
#include "Engine/EntityComponentManager.h"
#include "Engine/AssetSystem.h"

#include "Rendering/RenderSystem.h"

namespace zp
{
    class Engine
    {
    ZP_NONCOPYABLE( Engine )

    public:
        explicit Engine( MemoryLabel memoryLabel );

        ~Engine();

        [[nodiscard]] zp_bool_t isRunning() const;

        [[nodiscard]] zp_int32_t getExitCode() const;

        void processCommandLine( const String& cmdLine );

        void initialize();

        void destroy();

        void startEngine();

        void stopEngine();

        void restart();

        void exit( zp_int32_t exitCode );

        void process();

        void processWindowEvents();

        void advanceFrame();

        [[nodiscard]] zp_uint64_t getFrameCount() const
        {
            return m_frameCount;
        }

        [[nodiscard]] ModuleEntryPointAPI* getModuleAPI() const
        {
            return m_moduleAPI;
        }

        [[nodiscard]] RenderSystem* getRenderSystem() const
        {
            return m_renderSystem;
        }

        [[nodiscard]] EntityComponentManager* getEntityComponentManager() const
        {
            return m_entityComponentManager;
        }

        [[nodiscard]] AssetSystem* getAssetSystem() const
        {
            return m_assetSystem;
        }

    private:
        enum class EngineState : zp_uint32_t
        {
            Uninitialized,
            Initializing,
            Initialized,
            Running,
            Destroying,
            Destroyed,
            Exit,
        };

        void onStateEntered( EngineState engineState );

        void onStateProcess( EngineState engineState );

        void onStateExited( EngineState engineState );

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

        RenderSystem* m_renderSystem;
        EntityComponentManager* m_entityComponentManager;
        AssetSystem* m_assetSystem;

#if ZP_USE_PROFILER
        Profiler* m_profiler;
#endif

        zp_uint64_t m_frameCount;
        zp_time_t m_frameTime;
        zp_time_t m_timeFrequencyS;

        zp_int32_t m_restartCounter;
        zp_int32_t m_pauseCounter;
        zp_int32_t m_exitCode;

        EngineState m_nextEngineState;
        EngineState m_currentEngineState;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_ENGINE_H
