//
// Created by phosg on 11/10/2021.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"

#include "Engine/MemoryLabels.h"
#include "Engine/Engine.h"
#include "Engine/TransformComponent.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/GraphicsDevice.h"
#include <functional>

namespace zp
{
    Engine::Engine( MemoryLabel memoryLabel )
        : m_windowHandle( nullptr )
        , m_moduleDll( nullptr )
        , m_moduleAPI( nullptr )
        , m_windowCallbacks {}
        , m_currentEnginePipeline( nullptr )
        , m_nextEnginePipeline( nullptr )
        , m_previousFrameEnginePipelineHandle {}
        , m_jobSystem( nullptr )
        , m_renderSystem( nullptr )
        , m_entityComponentManager( nullptr )
#if ZP_USE_PROFILER
        , m_profiler( nullptr )
#endif
        , m_frameCount( 0 )
        , m_exitCode( 0 )
        , m_nextEngineState( Idle )
        , m_currentEngineState( Idle )
        , memoryLabel( memoryLabel )
    {
    }

    Engine::~Engine()
    {

    }

    zp_bool_t Engine::isRunning() const
    {
        return m_currentEngineState != Exit;
    }

    zp_int32_t Engine::getExitCode() const
    {
        return m_exitCode;
    }

    void Engine::initialize()
    {
        zp_handle_t mainThreadHandle = GetPlatform()->GetCurrentThread();
        GetPlatform()->SetThreadName( mainThreadHandle, "MainThread" );
        GetPlatform()->SetThreadIdealProcessor( mainThreadHandle, 0 );

#if ZP_USE_PROFILER
        m_profiler = ZP_NEW( Profiling, Profiler );

        Profiler::InitializeProfilerThread();
#endif

        const zp_uint32_t numJobThreads = 2; // GetPlatform()->GetProcessorCount() - 1;
        m_jobSystem = ZP_NEW_ARGS( Default, JobSystem, numJobThreads );

        m_renderSystem = ZP_NEW( Default, RenderSystem );

        m_entityComponentManager = ZP_NEW( Default, EntityComponentManager );

        //
        //
        //

        m_moduleDll = GetPlatform()->LoadExternalLibrary( "" );
        ZP_ASSERT( m_moduleDll );

        if( m_moduleDll )
        {
            auto getModuleAPI = GetPlatform()->GetProcAddress<GetModuleEntryPoint>( m_moduleDll, ZP_STR( GetModuleEntryPoint ) );
            ZP_ASSERT( getModuleAPI );

            if( getModuleAPI )
            {
                m_moduleAPI = getModuleAPI();
                ZP_ASSERT( m_moduleAPI );
            }
        }

        //
        //
        //

        if( m_moduleAPI ) m_moduleAPI->onEnginePreInitialize( this );

        m_windowCallbacks = {};
        m_windowCallbacks.minWidth = 320;
        m_windowCallbacks.minHeight = 240;
        m_windowCallbacks.maxWidth = 65535;
        m_windowCallbacks.maxHeight = 65535;
        m_windowCallbacks.onWindowResize = onWindowResize;

        OpenWindowDesc openWindowDesc {
            .callbacks = &m_windowCallbacks,
            .width = 800,
            .height = 600,
        };
        m_windowHandle = GetPlatform()->OpenWindow( &openWindowDesc );

        m_renderSystem->initialize( m_windowHandle, GraphicsDeviceFeatures::All );

        // Register components

        // Transform Components
        m_entityComponentManager->registerComponent<TransformComponentData>();
        m_entityComponentManager->registerComponent<RigidTransformComponentData>();
        m_entityComponentManager->registerComponent<UniformTransformComponentData>();
        m_entityComponentManager->registerComponent<PositionComponentData>();
        m_entityComponentManager->registerComponent<RotationComponentData>();
        m_entityComponentManager->registerComponent<UniformScaleComponentData>();
        m_entityComponentManager->registerComponent<ScaleComponentData>();
        m_entityComponentManager->registerComponent<ChildComponentData>();

        m_entityComponentManager->registerTag<DestroyedTag>();
        m_entityComponentManager->registerTag<DisabledTag>();
        m_entityComponentManager->registerTag<StaticTag>();

        // Render Components
        m_entityComponentManager->registerComponent<CameraComponentData>();

        if( m_moduleAPI ) m_moduleAPI->onEnginePostInitialize( this );
    }

    void Engine::destroy()
    {
        m_previousFrameEnginePipelineHandle.complete();
        m_previousFrameEnginePipelineHandle = JobHandle();

        if( m_moduleAPI ) m_moduleAPI->onEnginePreDestroy( this );

        m_renderSystem->destroy();

        if( m_moduleAPI ) m_moduleAPI->onEnginePostDestroy( this );

        //
        //
        //

        if( m_moduleDll )
        {
            GetPlatform()->UnloadExternalLibrary( m_moduleDll );
            m_moduleDll = nullptr;
            m_moduleAPI = nullptr;
        }

        if( m_windowHandle )
        {
            GetPlatform()->CloseWindow( m_windowHandle );
            m_windowHandle = nullptr;
        }

        ZP_SAFE_DELETE( EntityComponentManager, m_entityComponentManager );
        ZP_SAFE_DELETE( RenderSystem, m_renderSystem );
        ZP_SAFE_DELETE( JobSystem, m_jobSystem );

#if ZP_USE_PROFILER
        Profiler::DestroyProfilerThread();

        ZP_SAFE_DELETE( Profiler, m_profiler );
#endif
    }

    void Engine::startEngine()
    {
        m_nextEngineState = Initializing;

        if( m_moduleAPI )
        {
            m_moduleAPI->onEngineStarted( this );
        }
    }

    void Engine::stopEngine()
    {
        m_nextEngineState = Idle;

        if( m_moduleAPI )
        {
            m_moduleAPI->onEngineStopped( this );
        }
    }

    void Engine::restart()
    {
        m_nextEngineState = Restart;
    }

    void Engine::exit( zp_int32_t exitCode )
    {
        m_exitCode = exitCode;
        m_nextEngineState = Exit;
    }

    struct InitializationJob;

    struct StartJob;
    struct FixedUpdateJob;
    struct UpdateJob;
    struct LateUpdateJob;
    struct BeginFrameJob;
    struct EndFrameJob;

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
            PreparedJobHandle parentPreparedHandle = data->engine->getJobSystem()->Prepare( parentJobHandle );

            PreparedJobHandle renderHandle = data->engine->getJobSystem()->PrepareChildJob( parentPreparedHandle, nullptr );

           PreparedJobHandle renderSystemHandle = data->engine->getRenderSystem()->processSystem( data->engine->getFrameCount(), data->engine->getJobSystem(), parentPreparedHandle, renderHandle );

            EndFrameJob endFrameJob {
                data->engine
            };
            data->engine->getJobSystem()->PrepareJobData( endFrameJob, renderSystemHandle );

            data->engine->getJobSystem()->Schedule( renderHandle );
        }
    };


    struct LateUpdateJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const LateUpdateJob* data )
        {
            BeginFrameJob beginFrameJob {
                data->engine
            };
            data->engine->getJobSystem()->ScheduleJobData( beginFrameJob );
        }
    };

    struct UpdateJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const UpdateJob* data )
        {
            LateUpdateJob lateUpdateJob {
                data->engine
            };
            data->engine->getJobSystem()->ScheduleJobData( lateUpdateJob );
        }
    };


    struct FixedUpdateJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const FixedUpdateJob* data )
        {


            UpdateJob updateJob {
                data->engine
            };
            data->engine->getJobSystem()->ScheduleJobData( updateJob );
        }
    };

    struct StartJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const StartJob* data )
        {
            std::function func = [data]( Entity entity, const ComponentSignature& componentSignature ) -> void
            {
                data->engine->getEntityComponentManager()->destroyEntity( entity );
            };

            auto rr = func.target<EntityQueryCallback>();

            EntityQuery destroyedEntityQuery {};
            destroyedEntityQuery.tagsOnly = true;
            destroyedEntityQuery.componentSignature.addTag( data->engine->getEntityComponentManager()->getTagType<DestroyedTag>());

            //data->engine->getEntityComponentManager()->findEntities( &destroyedEntityQuery, *rr );

            FixedUpdateJob fixedUpdateJob {
                data->engine
            };
            data->engine->getJobSystem()->ScheduleJobData( fixedUpdateJob );
        }
    };

    void EndFrameJob::Execute( const JobHandle& parentJobHandle, const EndFrameJob* data )
    {
        data->engine->advanceFrame();

        StartJob startJob {
            data->engine
        };
        data->engine->getJobSystem()->ScheduleJobData( startJob );
    }


    struct InitializationJob
    {
        Engine* engine;

        static void Execute( const JobHandle& parentJobHandle, const InitializationJob* data )
        {
            StartJob startJob {
                data->engine
            };
            data->engine->getJobSystem()->ScheduleJobData( startJob );
        }
    };

    void Engine::process()
    {

        if( m_windowHandle )
        {
            zp_int32_t exitCode;
            const zp_bool_t isRunning = GetPlatform()->DispatchWindowMessages( m_windowHandle, &exitCode );
            if( !isRunning )
            {
                exit( exitCode );
            }
        }

        if( m_nextEngineState != m_currentEngineState )
        {
            m_currentEngineState = m_nextEngineState;
        }

        switch( m_currentEngineState )
        {
            case Idle:
                break;
            case Initializing:
            {
                InitializationJob initializationJob {
                    this
                };
                m_jobSystem->ScheduleJobData( initializationJob );

                m_nextEngineState = Running;
            }
                break;
            case Running:
            {
                zp_yield_current_thread();
            }
                break;
            case Exit:
                break;
        }

#if 0
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

                PreparedJobHandle frame = m_jobSystem->PrepareJob( nullptr );
                PreparedJobHandle initialFrameJob = frame;

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

        char windowTitle[64];
        zp_snprintf( windowTitle, "ZeroPoint 6 Frame:%d", m_frameCount );

        GetPlatform()->SetWindowTitle( m_windowHandle, windowTitle );

#if ZP_USE_PROFILER
        m_profiler->advanceFrame( m_frameCount );
#endif
    }
}