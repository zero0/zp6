//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_ENGINE_H
#define ZP_ENGINE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/Job.h"

#include "Platform/Platform.h"

#include "Engine/ModuleEntryPointAPI.h"
#include "Engine/EnginePipeline.h"
#include "Engine/Entity.h"
#include "Engine/Component.h"
#include "Engine/EntityComponentManager.h"
#include "Engine/AssetSystem.h"
#include "Engine/ExecutionGraph.h"
#include "Engine/Subsystem.h"

#include "Rendering/RenderSystem.h"

namespace zp
{
    class Engine
    {
    ZP_NONCOPYABLE( Engine )

    public:
        static Engine* GetInstance();

    public:
        explicit Engine( MemoryLabel memoryLabel );

        ~Engine();

        [[nodiscard]] zp_bool_t isRunning() const;

        [[nodiscard]] zp_int32_t getExitCode() const;

        void initialize( const String& cmdLine );

        void process();

        void shutdown();

        void startEngine();

        void stopEngine();

        void reload();

        void restart();

        void exit( zp_int32_t exitCode );

        void processWindowEvents();

        void advanceFrame();

        void update();

        [[nodiscard]] zp_uint64_t getFrameCount() const
        {
            return m_frameCount;
        }

        [[nodiscard]] ModuleEntryPointAPI* getModuleAPI() const
        {
            return m_moduleAPI;
        }

        [[nodiscard]] GraphicsDevice* getGraphicsDevice() const
        {
            return m_graphicsDevice;
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

        void submitToGPU();

    private:
        enum class EngineState : zp_uint32_t
        {
            Uninitialized,
            Running,
            Destroy,
            Reloading,
            Restarting,
            Exit,
        };

        void onStateEntered( EngineState engineState );

        void onStateProcess( EngineState engineState );

        void onStateExited( EngineState engineState );

        WindowHandle m_windowHandle;
        ConsoleHandle m_consoleHandle;

        zp_handle_t m_moduleDll;
        ModuleEntryPointAPI* m_moduleAPI;

        WindowCallbacks m_windowCallbacks;

        ExecutionGraph m_executionGraph;
        CompiledExecutionGraph m_compiledExecutionGraph;

        SubsystemManager m_subsystemManager;

        JobHandle m_previousFrameEnginePipelineHandle;

        GraphicsCommandBuffer* m_graphicsDeviceCommandBuffer;
        GraphicsDevice* m_graphicsDevice;
        RenderSystem* m_renderSystem;
        EntityComponentManager* m_entityComponentManager;
        AssetSystem* m_assetSystem;

        zp_uint64_t m_frameCount;
        zp_time_t m_frameStartTime;
        zp_time_t m_timeFrequencyS;

        ZP_BOOL32( m_shouldReload );
        ZP_BOOL32( m_shouldRestart );
        zp_int32_t m_pauseCounter;
        zp_int32_t m_exitCode;

        EngineState m_nextEngineState;
        EngineState m_currentEngineState;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_ENGINE_H
