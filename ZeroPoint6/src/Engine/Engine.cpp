//
// Created by phosg on 11/10/2021.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/String.h"
#include "Core/CommandLine.h"
#include "Core/Log.h"
#include "Core/Job.h"
#include "Core/Allocator.h"
#include "Core/Function.h"
#include "Core/Profiler.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"
#include "Engine/Engine.h"
#include "Engine/TransformComponent.h"
#include "Engine/AssetSystem.h"

#include "Rendering/Camera.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/GraphicsDevice.h"

//#include <functional>

namespace zp
{
    namespace
    {
        Engine* s_engine {};

        void OnWindowResize( zp_handle_t windowHandle, zp_int32_t width, zp_int32_t height, void* userPtr )
        {
#if 0
            std::function fff = [&](int a){ return width; };

            std::function iii( [](){} );

            std::function rrrrr = OnWindowResize;

            std::function ouu( OnWindowResize );


            Function eee = [&](int a){ return width; };

            Function yyy( [](){} );

            const Function uuu = OnWindowResize;

            Function pp( OnWindowResize );
#endif
        }

        void OnWindowHelpClosed( zp_handle_t windowHandle, void* userPtr )
        {
        }

        void OnWindowHelpEvent( zp_handle_t windowHandle, void* userPtr )
        {
            Engine::GetInstance()->reload();
        }
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
        , m_executionGraph( memoryLabel )
        , m_compiledExecutionGraph( memoryLabel )
        , m_subsystemManager( memoryLabel )
        , m_previousFrameEnginePipelineHandle {}
        , m_graphicsDevice( nullptr )
        , m_frameCount( 0 )
        , m_frameStartTime( 0 )
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

    void Engine::initialize( const String& cmdLine )
    {
        CommandLine cmd( MemoryLabels::Temp );
        const auto maxJobThreadParam = cmd.addOperation(
        {
            .shortName = {},
            .longName = String::As( "max-job-threads" ),
            .minParameterCount = 1,
            .maxParameterCount = 1,
            .type = CommandLineOperationParameterType::Int32,
        } );
        const auto headlessParam = cmd.addOperation(
        {
            .longName = String::As( "headless" ),
        } );

        if( !cmd.parse( cmdLine ) )
        {
            ZP_LOG_ERROR( "Failed to parse Command Line" );
            cmd.printHelp();
        }

        ZP_PROFILE_CPU_BLOCK_E( Initialize Engine );

        const zp_bool_t headless = cmd.hasFlag( headlessParam );
        const Rect2Di windowSize {
            .offset {},
            .size {
                .width = 800,
                .height = 600
            }
        };

        // create window
        const zp_bool_t createWindow = !headless;
        if( createWindow )
        {
            ZP_PROFILE_CPU_BLOCK_E( Create Window );

            m_windowCallbacks = {
                .minWidth = 320,
                .minHeight = 240,
                .maxWidth = 65535,
                .maxHeight = 65535,
                .onWindowResize = OnWindowResize,
                .onWindowHelpEvent = OnWindowHelpEvent,
                .onWindowClosed = OnWindowHelpClosed,
                .userPtr = this
            };

            m_windowHandle = Platform::OpenWindow(
            {
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
            ZP_PROFILE_CPU_BLOCK_E( Create Console );

            m_consoleHandle = Platform::OpenConsole();
        }

        // graphics device
        if( !headless )
        {
            ZP_PROFILE_CPU_BLOCK_E( Create Graphics Device );

            const zp_uint32_t bufferedFrameCount = 4;
            const zp_size_t stagingBufferSize = 32 MB;

            const GraphicsDeviceDesc desc {
                .appName = String::As( "AppName" ),
                .windowHandle = m_windowHandle,
                .stagingBufferSize = stagingBufferSize,
                .threadCount = JobSystem::GetThreadCount(),
                .bufferedFrameCount = bufferedFrameCount,
            };

            m_graphicsDevice = CreateGraphicsDevice( MemoryLabels::Graphics, desc );
            //m_graphicsDevice->createSwapchain( m_windowHandle.m_handle, windowSize.size.width, windowSize.size.height, 0, ZP_COLOR_SPACE_REC_709_LINEAR );
        }

        // show window when graphics device is created
        if( m_windowHandle )
        {
            Platform::ShowWindow( m_windowHandle, true );
        }

        // create systems
        m_subsystemManager.RegisterSubsystem<EntityComponentManager>( memoryLabel );

#if 0
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


        m_renderSystem->initialize( m_windowHandle.handle, {
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
#endif
    }

    namespace
    {
        struct BeginEngineJob
        {
            Engine* engine;

            static void Execute( const JobWorkArgs& args );
        };

        struct AdvanceFrameJob
        {
            Engine* engine;

            static void Execute( const JobWorkArgs& args )
            {
                Engine* engine = args.jobMemory.as<AdvanceFrameJob>()->engine;

                engine->advanceFrame();

                JobSystem::Start( BeginEngineJob { .engine = engine } );
            }
        };

        struct SyncGPUEngineJob
        {
            Engine* engine;

            static void Execute( const JobWorkArgs& args )
            {
                ZP_PROFILE_CPU_BLOCK();

                Engine* engine = args.jobMemory.as<SyncGPUEngineJob>()->engine;

                engine->submitToGPU();

                JobSystem::Start( AdvanceFrameJob { .engine = engine } );
            }
        };

        struct UpdateEngineJob
        {
            Engine* engine;

            static void Execute( const JobWorkArgs& args )
            {
                ZP_PROFILE_CPU_BLOCK();

                Engine* engine = args.jobMemory.as<UpdateEngineJob>()->engine;

                engine->update();

                JobSystem::Start( SyncGPUEngineJob { .engine = engine } );
            }
        };

        void BeginEngineJob::Execute( const zp::JobWorkArgs& args )
        {
            ZP_PROFILE_CPU_BLOCK();

            Engine* engine = args.jobMemory.as<BeginEngineJob>()->engine;

            JobSystem::Start( UpdateEngineJob { .engine = engine } );
        }

        struct InitialEngineJob
        {
            Engine* engine;

            static void Execute( const JobWorkArgs& args )
            {
                ZP_PROFILE_CPU_BLOCK();

                ZP_LOG_INFO( "Init job exec" );

                Engine* engine = args.jobMemory.as<InitialEngineJob>()->engine;

                JobSystem::Start( BeginEngineJob { .engine = engine } );
            }
        };
    }

    void Engine::startEngine()
    {

        if( m_moduleAPI )
        {
            m_moduleAPI->onEngineStarted( this );
        }

        JobSystem::Start( InitialEngineJob { .engine = this } );
    }

    void Engine::stopEngine()
    {
        JobSystem::Complete( m_previousFrameEnginePipelineHandle );
        m_previousFrameEnginePipelineHandle = {};


        if( m_moduleAPI )
        {
            m_moduleAPI->onEngineStopped( this );
        }
    }

    void Engine::shutdown()
    {
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
            m_windowHandle = {};
        }

        if( m_consoleHandle )
        {
            Platform::CloseConsole( m_consoleHandle );
            m_consoleHandle = {};
        }

        DestroyGraphicsDevice( m_graphicsDevice );
        m_graphicsDevice = nullptr;
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
            {
                zp_printfln( "Enter Uninitialized" );
            }
            break;

            case EngineState::Running:
            {
                zp_printfln( "Enter Running" );
                startEngine();
            }
            break;
            case EngineState::Destroy:
            {
                zp_printfln( "Enter Destroy" );
            }
            break;
            case EngineState::Reloading:
            {
                zp_printfln( "Enter Reloading" );
            }
            break;
            case EngineState::Restarting:
            {
                zp_printfln( "Enter Restarting" );
            }
            break;
            case EngineState::Exit:
            {
                zp_printfln( "Enter Exit" );
            }
            break;
        }
    }

    void Engine::onStateProcess( EngineState engineState )
    {
        switch( engineState )
        {
            case EngineState::Uninitialized:
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
                m_nextEngineState = EngineState::Running;
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
            case EngineState::Running:
            {
                zp_printfln( "Exit Running" );
                stopEngine();
            }
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

    void Engine::processWindowEvents()
    {
        zp_int32_t exitCode;

        if( m_windowHandle )
        {
            Platform::DispatchWindowMessages( m_windowHandle, exitCode );
        }

        const zp_bool_t isRunning = Platform::DispatchMessages( exitCode );
        if( !isRunning )
        {
            exit( exitCode );
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
                        zp_size_t frameCount;

                        static void Execute( const AdvanceProfilerFrameJob* data )
                        {
                            data->profiler->advanceFrame( data->frameCount );
                        }
                    } advanceProfilerFrame { m_profiler, m_frameCount };

                    frame = m_jobSystem->PrepareJobData( advanceProfilerFrame, frame );
                }
#endif

                struct TestJob
                {
                    zp_size_t frameCount;

                    static void Execute( const TestJob* data )
                    {
                        zp_printfln( "frame %d thread %d", data->frameCount, zp_current_thread_id());
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
        const zp_time_t totalCPUTime = now - m_frameStartTime;
        m_frameStartTime = now;

        const zp_float64_t durationMS = static_cast<zp_float64_t>(1000 * totalCPUTime) / static_cast<zp_float64_t>(m_timeFrequencyS);

        MutableFixedString128 windowTitle;
        windowTitle.format( "ZeroPoint 6 - Frame:%d (%f ms) T:(%d)", m_frameCount, durationMS, Platform::GetCurrentThreadId() );

        Platform::SetWindowTitle( m_windowHandle, windowTitle );

        ZP_PROFILE_ADVANCE_FRAME( m_frameCount );
    }

    void Engine::update()
    {
        zp_bool_t requiresRebuild = false;
        if( requiresRebuild )
        {
            m_executionGraph.BeginRecording();

            m_subsystemManager.BuildExecutionGraph( m_executionGraph );

            m_executionGraph.EndRecording();

            m_executionGraph.Compile( m_compiledExecutionGraph );

            requiresRebuild = false;
        }


#if 0
        RenderGraph graph;

        auto buff = graph.CreateBuffer();

        auto backBufferColor = graph.GetBackBufferColor();

        auto gbAlbedo = graph.CreateRenderTarget();
        auto rt0 = graph.CreateRenderTarget();
        auto rtd = graph.CreateRenderTarget();

        auto cb0 = graph.CreateBuffe( { .size = 1024 } );

        struct ConstantData
        {
            Matrix4f view;
            Vector4f projZ;
        } cdata;

        {
            ExecPassData* passData;
            auto builder = graph.AddExecutionPass<ExecPassData>( &passData );
            passData->buffer = builder.UseBuffer( buff, Write );
            passData->data = Memory { .ptr = &cdata, .size = sizeof(ConstantData) };
            builder.SetRenderFunc( []( ExecutionContext ctx, const ExecPassData* passData ){
                ctx.SetBufferData( passData->buffer, passData->data );
            } );
        }

        {
            ExecPassData* passData;
            auto builder = graph.AddComputePass<ExecPassData>( &passData );
            passData->buffer = builder.UseBuffer( buff, ReadWrite );
            passData->pipeline = builder.UseComputePipeline( cpipe );
            builder.SetRenderFunc( []( ExecutionContext ctx, const ExecPassData* passData ){
                ctx.SetPipeline( passData->pipeline );
                ctx.PushConstant( "push-name", {} );
                ctx.Dispatch( 8, 0, 0 );

                ctx.SetPipeline( passData->pipeline2 );
                ctx.PushConstant( "push-name", {} );
                ctx.Dispatch( 8, 0, 0 );
            } );
        }

        {
            PassData* passData;
            auto builder = graph.AddRasterPass<PassData>( &passData );
            passData->buffer = builder.UseBuffer( buff, Read );
            passData->renderList = builder.UseRenderList( {} );
            builder.SetColorAttachment( 0, gbAlbedo );
            builder.SetDepthAttachment( rtd );
            builder.SetRenderFunc( []( RasterContext ctx, const PassData* passData ){
                ctx.DrawRenderList( passData->renderList );
            } );
        }

        {
            OtherPassData* passData;
            auto builder = graph.AddRasterPass<OtherPassData>( &passData );
            passData->buffer = builder.UseBuffer( buff, Read );
            passData->pipeline = builder.UseRasterPipeline( pipeline );
            builder.SetInputAttachment( 0, gbAlbedo );
            builder.SetColorAttachment( 0, rt0 );
            builder.SetRenderFunc( []( RasterContext ctx, const OtherPassData* passData ){
                ctx.SetPipeline( passData->pipeline );
                ctx.Draw( 3, 1 );
            } );
        }

        {
            auto builder = graph.AddBlitPass();
            builder.SetSrcTexture( rt0, mip: 0, face: CubemapFace::None, srcUV: {0, 0, 1, 1} );
            builder.SetDstTexture( backBufferColor, mip: 0, face: CubemapFace::None, dstUV: {0, 0, 1, 1} );
        }

        RenderGraphCache renderGraphCache;

        CompiledRenderGraph compiledRenderGraph;
        if( !renderGraphCache.TryGet( graph, compiledRenderGraph ) )
        {
            bool compiled = graph.Compile( CompilerFlags::None, compiledRenderGraph );
            renderGraphCache.Add( graph, compiledRenderGraph );
        }


        if( compiledRenderGraph.valid() )
        {
            compiledRenderGraph.Execute( graphicsCommandBuffer );
        }
        else
        {
            renderGraph.Execute( graphicsCommandBuffer );
        }

        //
        //
        //

        struct ConstantBufferData {
            Matrix4x4f view;
            Vector4f pos;
        } cb;

        BufferHandle buff{};
        buff = m_graphicsDeviceCommandBuffer->RequestBuffer( buff, { .size = 1024 } );

        auto cmdQueue = m_graphicsDeviceCommandBuffer->BeginCommandQueue();

        m_graphicsDeviceCommandBuffer->UpdateBuffer( cmdQueue, buff, 0, cb );

        m_graphicsDeviceCommandBuffer->Draw( cmdQueue, 3, 1, 0, 0 );

        m_graphicsDeviceCommandBuffer->SubmitCommandQueue( cmdQueue );
#endif
    }

    void Engine::submitToGPU()
    {
        GraphicsCommandBuffer* graphicsCommandBuffer = m_graphicsDevice->RequestCommandBuffer();

        {
            auto commandBuffer = graphicsCommandBuffer->BeginCommandBuffer( ZP_RENDER_QUEUE_GRAPHICS );


#if 1
            auto pipeline = graphicsCommandBuffer->RequestPipeline( {} );

            auto buffer = graphicsCommandBuffer->RequestBuffer( {}, {} );

            graphicsCommandBuffer->PushConstant( commandBuffer, pipeline, 0, 0, {} );

            graphicsCommandBuffer->BeginRenderPass( commandBuffer, {} );

            graphicsCommandBuffer->BindPipeline( commandBuffer, {} );

            graphicsCommandBuffer->Draw( commandBuffer, 3, 1, 0, 0 );

            graphicsCommandBuffer->EndRenderPass( commandBuffer, {} );
#endif
            graphicsCommandBuffer->SubmitCommandBuffer( commandBuffer );
        }

        m_graphicsDevice->SubmitCommandBuffer( graphicsCommandBuffer );

        //m_graphicsDeviceCommandBuffer = m_graphicsDevice->SubmitAndRequestNewCommandBuffer( m_graphicsDeviceCommandBuffer );
    }
}
