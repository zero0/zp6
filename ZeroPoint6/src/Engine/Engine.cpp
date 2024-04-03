//
// Created by phosg on 11/10/2021.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/String.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"
#include "Engine/Engine.h"
#include "Engine/TransformComponent.h"
#include "Engine/AssetSystem.h"

#include "Rendering/Camera.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/GraphicsDevice.h"

namespace zp
{
    namespace
    {
        Engine* s_engine = {};
    }

    Engine* Engine::GetInstance()
    {
        return s_engine;
    }

    Engine::Engine( MemoryLabel memoryLabel )
        : m_windowHandle( nullptr )
        , m_consoleHandle( nullptr )
        , m_moduleDll( nullptr )
        , m_moduleAPI( nullptr )
        , m_windowCallbacks {}
        , m_currentEnginePipeline( nullptr )
        , m_nextEnginePipeline( nullptr )
        , m_previousFrameEnginePipelineHandle {}
        , m_graphicsDevice( nullptr )
        , m_renderSystem( nullptr )
        , m_entityComponentManager( nullptr )
        , m_assetSystem( nullptr )
#if ZP_USE_PROFILER
        , m_profiler( nullptr )
#endif
        , m_frameCount( 0 )
        , m_frameTime( 0 )
        , m_timeFrequencyS( Platform::TimeFrequency() )
        , m_shouldReload( false )
        , m_shouldRestart( false )
        , m_pauseCounter( 0 )
        , m_exitCode( 0 )
        , m_nextEngineState( EngineState::Uninitialized )
        , m_currentEngineState( EngineState::Uninitialized )
        , memoryLabel( memoryLabel )
    {
        ZP_ASSERT( s_engine == nullptr );
        s_engine = this;
    }

    Engine::~Engine()
    {
        ZP_ASSERT( s_engine == this );
        s_engine = nullptr;
    }

    zp_bool_t Engine::isRunning() const
    {
        return m_currentEngineState != EngineState::Exit;
    }

    zp_int32_t Engine::getExitCode() const
    {
        return m_exitCode;
    }

    void DestroyRawAssetComponentDataCallback( void* componentData, zp_size_t componentSize )
    {

    }

    void Engine::processCommandLine( const String& cmdLine )
    {

    }

    void Engine::initialize()
    {
        zp_handle_t mainThreadHandle = Platform::GetCurrentThread();
        Platform::SetThreadName( mainThreadHandle, String::As( "MainThread" ) );
        Platform::SetThreadIdealProcessor( mainThreadHandle, 0 );

        const zp_uint32_t numJobThreads = 2; // Platform::GetProcessorCount() - 1;

#if ZP_USE_PROFILER
        m_profiler = ZP_NEW_ARGS( MemoryLabels::Profiling, Profiler, {
            .maxThreadCount = numJobThreads + 1,
            .maxCPUEventsPerThread = 128,
            .maxMemoryEventsPerThread = 128,
            .maxGPUEventsPerThread = 4,
            .maxFramesToCapture = 120,
        } );

        Profiler::InitializeProfilerThread();
#endif

        ZP_PROFILE_CPU_BLOCK_E(Initialize Engine);

        // initialize job system
        JobSystem::Setup( MemoryLabels::Default, numJobThreads );

        const zp_bool_t headless = false;
        const Rect2Di windowSize {
            .offset {},
            .size {
                .width = 800,
                .height = 600
            }
        };

        // create window
        const zp_bool_t createWindow = true;
        if( createWindow )
        {
            ZP_PROFILE_CPU_BLOCK_E(Create Window);

            m_windowCallbacks = {
                .minWidth = 320,
                .minHeight = 240,
                .maxWidth = 65535,
                .maxHeight = 65535,
                .onWindowResize = onWindowResize,
                .onWindowHelpEvent = onWindowHelpEvent
            };

            m_windowHandle = Platform::OpenWindow( {
                .callbacks = &m_windowCallbacks,
                .width = windowSize.size.width,
                .height = windowSize.size.height,
                .showWindow = false
            } );
        }

        // create console
        const zp_bool_t createConsole = true;
        if( createConsole )
        {
            ZP_PROFILE_CPU_BLOCK_E(Create Console);

            m_consoleHandle = Platform::OpenConsole();
        }

        // graphics device
        if( !headless )
        {
            ZP_PROFILE_CPU_BLOCK_E(Create Graphics Device);

            m_graphicsDevice = CreateGraphicsDevice( MemoryLabels::Graphics, {
                .appName = String::As( "AppName" ),
                .stagingBufferSize = 32 MB,
                .threadCount = numJobThreads,
                .bufferedFrameCount = 4,
            } );

            m_graphicsDevice->createSwapChain( m_windowHandle, windowSize.size.width, windowSize.size.height, 0, ZP_COLOR_SPACE_REC_709_LINEAR );
        }
        // create systems
        //m_renderSystem = ZP_NEW( MemoryLabels::Default, RenderSystem );

        // show window when graphics device is created
        Platform::ShowWindow(m_windowHandle, true );

        return;
        const char* moduleDLLPath = "";
        if( !zp_strempty( moduleDLLPath ) )
        {
            m_moduleDll = Platform::LoadExternalLibrary( "" );
            ZP_ASSERT_MSG( m_moduleDll, "Unable to load module" );

            if( m_moduleDll )
            {
                auto getModuleAPI = Platform::GetProcAddress<GetModuleEntryPoint>( m_moduleDll, ZP_STR_NAMEOF( GetModuleEntryPoint ) );
                ZP_ASSERT_MSG( getModuleAPI, "Unable to find " ZP_NAMEOF( GetModuleEntryPoint ) );

                if( getModuleAPI )
                {
                    m_moduleAPI = getModuleAPI();
                    ZP_ASSERT_MSG( m_moduleAPI, "No " ZP_NAMEOF( ModuleEntryPointAPI ) " returned" );
                }
            }
        }

        //
        //
        //


        m_entityComponentManager = ZP_NEW( MemoryLabels::Default, EntityComponentManager );

        m_assetSystem = ZP_NEW( MemoryLabels::Default, AssetSystem );

        //
        //
        //

        if( m_moduleAPI )
        {
            m_moduleAPI->onEnginePreInitialize( this );
        }


        m_renderSystem->initialize( m_windowHandle, {
            .appName = ZP_STR_T( "AppName" ),
            .stagingBufferSize = 32 MB,
            .threadCount = numJobThreads,
            .bufferedFrameCount = 4,
            .geometryShaderSupport = true,
            .tessellationShaderSupport = true,
        } );

        m_assetSystem->setup( m_entityComponentManager );

        // Register components

        // Asset Components
        m_entityComponentManager->registerComponent<AssetReferenceComponentData<0>>();
        m_entityComponentManager->registerComponent<RawAssetComponentData>( DestroyRawAssetComponentDataCallback );
        m_entityComponentManager->registerComponent<AssetReferenceCountComponentData>();
        m_entityComponentManager->registerComponent<MeshAssetViewComponentData>();

        // Transform Components
        m_entityComponentManager->registerComponent<TransformComponentData>();
        m_entityComponentManager->registerComponent<RigidTransformComponentData>();
        m_entityComponentManager->registerComponent<UniformTransformComponentData>();
        m_entityComponentManager->registerComponent<PositionComponentData>();
        m_entityComponentManager->registerComponent<RotationComponentData>();
        m_entityComponentManager->registerComponent<UniformScaleComponentData>();
        m_entityComponentManager->registerComponent<ScaleComponentData>();
        m_entityComponentManager->registerComponent<ChildComponentData>();

        // Common Tags
        m_entityComponentManager->registerTag<DestroyedTag>();
        m_entityComponentManager->registerTag<DisabledTag>();
        m_entityComponentManager->registerTag<StaticTag>();
        m_entityComponentManager->registerTag<MainCameraTag>();

        // Render Components
        m_entityComponentManager->registerComponent<CameraComponentData>();

        // Generic signatures

        // Main Camera Signature
        m_entityComponentManager->registerComponentSignature( {
            .tagSignature = m_entityComponentManager->getTagSignature<MainCameraTag>(),
            .structuralSignature = m_entityComponentManager->getComponentSignature<CameraComponentData, RigidTransformComponentData>(),
        } );

        // Asset Signature
        m_entityComponentManager->registerComponentSignature( {
            .structuralSignature = m_entityComponentManager->getComponentSignature<RawAssetComponentData>()
        } );

        m_entityComponentManager->registerComponentSignature( {
            .structuralSignature = m_entityComponentManager->getComponentSignature<RawAssetComponentData, AssetReferenceCountComponentData>()
        } );

        if( m_moduleAPI )
        {
            m_moduleAPI->onEnginePostInitialize( this );
        }
    }

    namespace
    {
        struct EngineJobData
        {
            Engine* engine;
        };

        void StartFrameJob( const JobHandle& parentHandle, EngineJobData* e );

        void AdvanceFrameJob( const JobHandle& parentHandle, EngineJobData* e )
        {
            if( e->engine->isRunning() )
            {
                e->engine->advanceFrame();

                ZP_PROFILE_ADVANCE_FRAME( e->engine->getFrameCount() );

                JobSystem::Start( StartFrameJob, *e ).schedule();
                JobSystem::ScheduleBatchJobs();
            }
        }

        void EndFrameJob( const JobHandle& parentHandle, EngineJobData* e )
        {
            zp_printfln( "[%10d] EndFrameJob", Platform::GetCurrentThreadId() );

            JobSystem::Start( AdvanceFrameJob, *e ).schedule();
            JobSystem::ScheduleBatchJobs();
        }

        void StartFrameJob( const JobHandle& parentHandle, EngineJobData* e )
        {
            zp_printfln( "[%10d] StartFrameJob", Platform::GetCurrentThreadId() );

            JobSystem::Start( EndFrameJob, *e ).schedule();
            JobSystem::ScheduleBatchJobs();
        }

        void InitialEngineJob( const JobHandle& parentHandle, EngineJobData* e )
        {
            JobSystem::Start( StartFrameJob, *e ).schedule();
            JobSystem::ScheduleBatchJobs();
        }
    }

    void Engine::startEngine()
    {
        JobSystem::InitializeJobThreads();

        if( m_moduleAPI )
        {
            m_moduleAPI->onEngineStarted( this );
        }

        JobSystem::Start( InitialEngineJob, { .engine = this } ).schedule();
        JobSystem::ScheduleBatchJobs();
    }

    void Engine::stopEngine()
    {
        JobSystem::ExitJobThreads();

        if( m_moduleAPI )
        {
            m_moduleAPI->onEngineStopped( this );
        }
    }

    void Engine::destroy()
    {
        m_previousFrameEnginePipelineHandle.complete();
        m_previousFrameEnginePipelineHandle = {};

        //m_assetSystem->teardown();

        if( m_moduleAPI )
        {
            m_moduleAPI->onEnginePreDestroy( this );
        }

        //m_renderSystem->destroy();

        if( m_moduleAPI )
        {
            m_moduleAPI->onEnginePostDestroy( this );
        }

        //
        //
        //

        if( m_moduleDll )
        {
            Platform::UnloadExternalLibrary( m_moduleDll );
            m_moduleDll = nullptr;
            m_moduleAPI = nullptr;
        }

        if( m_windowHandle )
        {
            Platform::CloseWindow( m_windowHandle );
            m_windowHandle = nullptr;
        }

        if( m_consoleHandle )
        {
            Platform::CloseConsole( m_consoleHandle );
            m_consoleHandle = nullptr;
        }

        DestroyGraphicsDevice( m_graphicsDevice );
        m_graphicsDevice = nullptr;

        ZP_SAFE_DELETE( RenderSystem, m_renderSystem );
        ZP_SAFE_DELETE( EntityComponentManager, m_entityComponentManager );
        ZP_SAFE_DELETE( AssetSystem, m_assetSystem );

#if ZP_USE_PROFILER
        Profiler::DestroyProfilerThread();

        ZP_SAFE_DELETE( Profiler, m_profiler );
#endif

        JobSystem::Teardown();
    }

    void Engine::reload()
    {
        m_shouldReload = true;
        m_nextEngineState = EngineState::Destroy;
    }

    void Engine::restart()
    {
        m_shouldRestart = true;
        m_nextEngineState = EngineState::Destroy;
    }

    void Engine::exit( zp_int32_t exitCode )
    {
        m_shouldReload = false;
        m_shouldRestart = false;
        m_exitCode = exitCode;
        m_nextEngineState = EngineState::Destroy;
    }

    void Engine::onStateEntered( EngineState engineState )
    {
        switch( engineState )
        {
            case EngineState::Uninitialized:
                zp_printfln( "Enter Uninitialized" );
                break;
            case EngineState::Initialize:
            {
                initialize();
            }
                zp_printfln( "Enter Initialize" );
                break;
            case EngineState::Running:
            {
                startEngine();
            }
                zp_printfln( "Enter Running" );
                break;
            case EngineState::Destroy:
            {

            }
                zp_printfln( "Enter Destroy" );
                break;
            case EngineState::Reloading:
            {

            }
                zp_printfln( "Enter Reloading" );
                break;
            case EngineState::Restarting:
            {
                destroy();
            }
                zp_printfln( "Enter Restarting" );
                break;
            case EngineState::Exit:
            {
                destroy();
            }
                zp_printfln( "Enter Exit" );
                break;
        }
    }

    void Engine::onStateProcess( EngineState engineState )
    {
        switch( engineState )
        {
            case EngineState::Uninitialized:
            {
                m_nextEngineState = EngineState::Initialize;
            }
                break;
            case EngineState::Initialize:
            {
                m_nextEngineState = EngineState::Running;
            }
                break;
            case EngineState::Running:
            {
                JobSystem::ProcessJobs();

                Platform::YieldCurrentThread();
            }
                break;
            case EngineState::Destroy:
            {
                if( m_shouldReload )
                {
                    m_nextEngineState = EngineState::Reloading;
                }
                else if( m_shouldRestart )
                {
                    m_nextEngineState = EngineState::Restarting;
                }
                else
                {
                    m_nextEngineState = EngineState::Exit;
                }
            }
                break;
            case EngineState::Reloading:
            {
                m_nextEngineState = EngineState::Running;
            }
                break;
            case EngineState::Restarting:
            {
                m_nextEngineState = EngineState::Initialize;
            }
                break;
            case EngineState::Exit:
                break;
        }
    }

    void Engine::onStateExited( EngineState engineState )
    {
        switch( engineState )
        {
            case EngineState::Uninitialized:
                zp_printfln( "Exit Uninitialized" );
                break;
            case EngineState::Initialize:
                zp_printfln( "Exit Initialize" );
                break;
            case EngineState::Running:
            {
                stopEngine();
            }
                zp_printfln( "Exit Running" );
                break;
            case EngineState::Destroy:
                zp_printfln( "Exit Destroy" );
                break;
            case EngineState::Reloading:
                zp_printfln( "Exit Reloading" );
                break;
            case EngineState::Restarting:
                zp_printfln( "Exit Restarting" );
                break;
            case EngineState::Exit:
                zp_printfln( "Exit Exit" );
                break;
        }
    }

    struct InitializeEngineJob;
    struct InitializeModuleJob;
    struct ProcessWindowEventsJob;

    struct StartJob;
    struct FixedUpdateJob;
    struct UpdateJob;
    struct LateUpdateJob;
    struct BeginFrameJob;
    struct EndFrameJob;

#if ZP_USE_PROFILER
    struct AdvanceProfilerFrameJob;
#endif

    struct EndFrameJob
    {
        Engine* engine;


        static void Execute( const JobHandle& parentJobHandle, const EndFrameJob* data );
    };

    struct BeginFrameJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const BeginFrameJob* data )
        {
            ZP_PROFILE_CPU_BLOCK();

        }
    };

    struct LateUpdateJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const LateUpdateJob* data )
        {
            ZP_PROFILE_CPU_BLOCK();

        }
    };

    struct UpdateJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const UpdateJob* data )
        {
            ZP_PROFILE_CPU_BLOCK();

            EntityComponentManager* entityComponentManager = data->engine->getEntityComponentManager();

            EntityQueryIterator iterator {};
            entityComponentManager->iterateEntities( {
                .notIncludedTags = entityComponentManager->getTagSignature<DisabledTag>(),
                .requiredStructures = entityComponentManager->getComponentSignature<TransformComponentData, ChildComponentData>(),
            }, &iterator );

            while( iterator.next() )
            {
            };

        }
    };


    struct FixedUpdateJob
    {
        Engine* engine;


        static void Execute( const JobHandle& parentJobHandle, const FixedUpdateJob* data )
        {
            ZP_PROFILE_CPU_BLOCK();

            EntityComponentManager* entityComponentManager = data->engine->getEntityComponentManager();

            EntityQueryIterator iterator {};
            entityComponentManager->iterateEntities( {
                .notIncludedTags = entityComponentManager->getTagSignature<DisabledTag>(),
                .requiredStructures = entityComponentManager->getComponentSignature<TransformComponentData>(),
            }, &iterator );

            while( iterator.next() )
            {
            };


        }
    };

    struct StartJob
    {
        Engine* engine;


        static void Execute( const JobHandle& parentJobHandle, const StartJob* data )
        {
            ZP_PROFILE_CPU_BLOCK();

            EntityComponentManager* entityComponentManager = data->engine->getEntityComponentManager();

            // replay command buffers
            entityComponentManager->replayCommandBuffers();

            // destroy tagged entities
            EntityQueryIterator iterator {};
            entityComponentManager->iterateEntities( {
                .requiredTags = entityComponentManager->getTagSignature<DestroyedTag>(),
            }, &iterator );

            while( iterator.next() )
            {
                iterator.destroyEntity();
            };

        }
    };

#if ZP_USE_PROFILER

    struct AdvanceProfilerFrameJob
    {
        Engine* engine;


        static void Execute( const JobHandle& parentJobHandle, const AdvanceProfilerFrameJob* data )
        {
            ZP_PROFILE_ADVANCE_FRAME( data->engine->getFrameCount() );


        }
    };

#endif

    void EndFrameJob::Execute( const JobHandle& parentJobHandle, const EndFrameJob* data )
    {
        if( data->engine->isRunning() )
        {
            data->engine->advanceFrame();


        }
    }

    struct InitializeModuleJob
    {
        Engine* engine;


        static void Execute( const JobHandle& parentJobHandle, const InitializeModuleJob* data )
        {

        }
    };

    struct InitializeEngineJob
    {
        Engine* engine;


        static void Execute( const JobHandle& parentJobHandle, const InitializeEngineJob* data )
        {
        }
    };

    void Engine::processWindowEvents()
    {
        //if( m_windowHandle )
        {
            zp_int32_t exitCode;
            const zp_bool_t isRunning = Platform::DispatchMessages( exitCode );
            if( !isRunning )
            {
                exit( exitCode );
            }
        }
    }

    void Engine::process()
    {
        processWindowEvents();

        const zp_bool_t stateChanged = m_nextEngineState != m_currentEngineState;
        if( stateChanged )
        {
            onStateExited( m_currentEngineState );

            m_currentEngineState = m_nextEngineState;

            onStateEntered( m_currentEngineState );
        }

        onStateProcess( m_currentEngineState );

#if 0
        if( stateChanged )
        {
            switch( m_currentEngineState )
            {
                case EngineState::Uninitialized:
                {
                    m_nextEngineState = EngineState::Initializing;
                }
                    break;

                case EngineState::Initializing:
                {
                    m_nextEngineState = EngineState::Initialized;
                }
                    break;

                case EngineState::Initialized:
                {
                    m_nextEngineState = EngineState::Running;
                }
                    break;

                case EngineState::Running:
                {
                    zp_yield_current_thread();
                }
                    break;

                case EngineState::Paused:
                {
                    zp_yield_current_thread();
                }
                    break;

                case EngineState::Restart:
                {
                    stopEngine();

                    destroy();

                    initialize();

                    startEngine();
                }
                    break;

                case EngineState::Destroying:
                {
                    m_nextEngineState = EngineState::Destroyed;
                }
                    break;

                case EngineState::Destroyed:
                {
                    m_nextEngineState = EngineState::Uninitialized;
                }
                    break;

                case EngineState::Exit:
                {
                    zp_printfln( "Exit" );
                }
                    break;
            }
        }

        switch( m_currentEngineState )
        {
            case Running:
            {
                ZP_PROFILE_CPU_BLOCK_E( "Engine - Running" );

                ++m_frameCount;

                if( m_nextEnginePipeline != nullptr )
                {
                    m_previousFrameEnginePipelineHandle.complete();
                    m_previousFrameEnginePipelineHandle = {};

                    if( m_currentEnginePipeline ) m_currentEnginePipeline->onDeactivated();

                    m_currentEnginePipeline = m_nextEnginePipeline;
                    m_nextEnginePipeline = nullptr;

                    if( m_currentEnginePipeline ) m_currentEnginePipeline->onActivated();
                }

                m_previousFrameEnginePipelineHandle.complete();

                JobHandle frame = m_jobSystem->PrepareJob( nullptr );
                JobHandle initialFrameJob = frame;

                if( m_currentEnginePipeline )
                {
                    ZP_PROFILE_CPU_BLOCK_E( "Engine - Process Engine Pipeline" );

                    frame = m_currentEnginePipeline->onProcessPipeline( this, frame );
                }

                if( m_renderSystem )
                {
                    ZP_PROFILE_CPU_BLOCK_E( "Engine - Process Render System" );

                    frame = m_renderSystem->processSystem( m_jobSystem, frame );
                }

#if ZP_USE_PROFILER
                if( m_profiler )
                {
                    struct AdvanceProfilerFrameJob
                    {
                        Profiler* profiler;
                        zp_size_t frameIndex;

                        static void Execute( const AdvanceProfilerFrameJob* data )
                        {
                            data->profiler->advanceFrame( data->frameIndex );
                        }
                    } advanceProfilerFrame { m_profiler, m_frameCount };

                    frame = m_jobSystem->PrepareJobData( advanceProfilerFrame, frame );
                }
#endif

                struct TestJob
                {
                    zp_size_t frameIndex;

                    static void Execute( const TestJob* data )
                    {
                        zp_printfln( "frame %d thread %d", data->frameIndex, zp_current_thread_id());
                    }
                } testJob { m_frameCount };

                m_jobSystem->PrepareJobData( testJob, frame );

                m_previousFrameEnginePipelineHandle = m_jobSystem->Schedule( initialFrameJob );

            }
                break;
            case Initializing:
            {

            }
                break;
            case Idle:
            {
                m_previousFrameEnginePipelineHandle.complete();
                m_previousFrameEnginePipelineHandle = {};
            }
                break;

            case Restart:
            {
                stopEngine();

                destroy();

                initialize();

                startEngine();
            }
                break;

            case Exit:
            {
                m_previousFrameEnginePipelineHandle.complete();
                m_previousFrameEnginePipelineHandle = JobHandle();
            }
                break;
        }
#endif
    }

    void Engine::advanceFrame()
    {
        ++m_frameCount;

        const zp_time_t now = Platform::TimeNow();
        const zp_time_t totalCPUTime = now - m_frameTime;
        m_frameTime = now;

        const zp_float64_t durationMS = static_cast<zp_float64_t>( 1000 * totalCPUTime ) / static_cast<zp_float64_t>( m_timeFrequencyS );

        MutableFixedString<128> windowTitle;
        windowTitle.format( "ZeroPoint 6 - Frame:%d (%f ms) T:(%d)", m_frameCount, durationMS, Platform::GetCurrentThreadId() );

        Platform::SetWindowTitle( m_windowHandle, windowTitle );

        ZP_PROFILE_ADVANCE_FRAME( m_frameCount );
    }
}
