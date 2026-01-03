//
// Created by phosg on 7/16/2023.
//

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Memory.h"
#include "Core/CommandLine.h"
#include "Core/Log.h"
#include "Core/Http.h"
#include "Core/Profiler.h"

#include "Platform/Platform.h"

#include "Rendering/Shader.h"

#include "Tools/AssetCompiler.h"
#include "Tools/ShaderCompiler.h"


using namespace zp;


void AssetCompilerTask::Execute( const zp::JobHandle& jobHandle, zp::AssetCompilerTask* task )
{
    if( task->jobExec )
    {
        task->jobExec( jobHandle, task );
    }
    else if( task->exec )
    {
        task->exec( task );
    }
}

AssetCompiler::AssetCompiler( MemoryLabel memoryLabel )
    : m_assetProcessors( memoryLabel, 16, memoryLabel )
    , memoryLabel( memoryLabel )
{
}

AssetCompiler::~AssetCompiler()
{
}

void AssetCompiler::registerFileExtension( const String& ext, const AssetCompilerProcessor& assetProcessor )
{
    m_assetProcessors.set( ext, assetProcessor );
}

const AssetCompilerProcessor* AssetCompiler::getCompilerProcessor( const String& ext ) const
{
    AssetCompilerProcessor* assetCompilerProcessor;
    return m_assetProcessors.tryGet( ext, &assetCompilerProcessor ) ? assetCompilerProcessor : nullptr;
}

AssetCompilerApplication::AssetCompilerApplication( MemoryLabel memoryLabel )
    : m_assetCompiler( memoryLabel )
    , m_exitCode( 0 )
    , m_isRunning( false )
    , m_infoPort( 8080 )
    , m_assetPort( 40000 )
    , memoryLabel( memoryLabel )
{
}

void AssetCompilerApplication::initialize( const String& commandLine )
{
    zp_size_t numJobThreads = 2;

    CommandLine cmdLine( MemoryLabels::Default );
    if( cmdLine.parse( commandLine.c_str() ) )
    {
        const CommandLineOperation versionOp = cmdLine.addOperation( {
            .shortName = String::As( "-v" ),
            .longName = String::As( "--version" ),
        } );
        const CommandLineOperation helpOp = cmdLine.addOperation( {
            .shortName = String::As( "-h" ),
            .longName = String::As( "--help" )
        } );

        const CommandLineOperation infoPortOp = cmdLine.addOperation( {
            .longName = String::As( "--info-port" ),
            .minParameterCount = 1,
            .maxParameterCount = 1,
        } );

        const CommandLineOperation assetPortOp = cmdLine.addOperation( {
            .longName = String::As( "--asset-port" ),
            .minParameterCount = 1,
            .maxParameterCount = 1,
        } );

        const CommandLineOperation libraryOp = cmdLine.addOperation( {
            .longName = String::As( "--library-path" ),
            .minParameterCount = 1,
            .maxParameterCount = 1,
        } );

        const CommandLineOperation jobThreadCountOp = cmdLine.addOperation( {
            .longName = String::As( "--job-count" ),
            .minParameterCount = 1,
            .maxParameterCount = 1,
            .type = CommandLineOperationParameterType::Int32,
        } );
        //
        // Operations
        //

        if( cmdLine.hasFlag( versionOp ) )
        {
            Log::message() << "Version: " << "???" << Log::endl;
            requestExit( 0 );
            return;
        }

        if( cmdLine.hasFlag( helpOp ) )
        {
            cmdLine.printHelp();
            requestExit( 0 );
            return;
        }

        zp_int32_t infoPort;
        if( cmdLine.tryGetParameterAsInt32( infoPortOp, infoPort ) )
        {
            m_infoPort = infoPort;
        }

        zp_int32_t assetPort;
        if( cmdLine.tryGetParameterAsInt32( assetPortOp, assetPort ) )
        {
            m_assetPort = assetPort;
        }

        Vector<String> libraries;
        if( cmdLine.hasParameter( libraryOp, libraries ) )
        {

        }

        zp_int32_t jobThreadCount;
        if( cmdLine.tryGetParameterAsInt32( jobThreadCountOp, jobThreadCount ) )
        {
            numJobThreads = zp_max( jobThreadCount, 1 );
        }
    }
    else
    {
        ZP_ASSERT_MSG_ARGS( false, "Failed to parse command line: %s", commandLine.c_str() );
    }

#if ZP_USE_PROFILER
    Profiler::CreateProfiler( MemoryLabels::Profiling, {
        .maxThreadCount = numJobThreads + 2, // main thread + socket thread
        .maxCPUEventsPerThread = 128,
        .maxMemoryEventsPerThread = 128,
        .maxGPUEventsPerThread = 4,
        .maxFramesToCapture = 120,
    } );

    Profiler::InitializeProfilerThread();
#endif

    // initialize job system
    JobSystem::Setup( MemoryLabels::Default, numJobThreads );

    Platform::InitializeNetworking();

    ShaderCompiler::Initialize();

    Log::info() << "Register File Extensions..." << Log::endl;

    const OpenSystemTrayDesc openSystemTrayDesc {};
    m_systemTray = Platform::OpenSystemTray( openSystemTrayDesc );

    // register file extensions
    m_assetCompiler.registerFileExtension( String::As( ".shader" ), {
        //.createTaskFunc = ShaderCompiler::ShaderCompilerCreateTaskMemory,
        //.deleteTaskFunc = ShaderCompiler::ShaderCompilerDestroyTaskMemory,
        //.executeFunc = nullptr, //ShaderCompiler::ShaderCompilerExecute,
        //.jobExecuteFunc = ShaderCompiler::ShaderCompilerExecuteJob,
    } );
    m_assetCompiler.registerFileExtension( String::As( ".compute" ), {} );

    m_assetCompiler.registerFileExtension( String::As( ".tiff" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".png" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".jpg" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".jpeg" ), {} );

    m_assetCompiler.registerFileExtension( String::As( ".prefab" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".txt" ), {} );

    Log::info() << "Asset Port: " << m_assetPort << Log::endl;

    m_infoSocket = Platform::OpenSocket( {
        .name = "AssetCompiler::InfoSocket",
        .address = IPAddress::Localhost( m_infoPort ),
        .addressFamily = AddressFamily::IPv4,
        .socketType = SocketType::Stream,
        .connectionProtocol = ConnectionProtocol::TCP,
        .socketDirection = SocketDirection::Listen,
    } );

    zp_uint32_t threadId;
    m_receiveThread = Platform::CreateThread( ReceiveInfoSocketThreadFunc, this, 1 MB, &threadId );

    Log::info() << "Info Listening on Port:  " << m_infoPort << " (" << threadId << ")" << Log::endl;

    m_isRunning = true;
}

zp_uint32_t AssetCompilerApplication::ReceiveInfoSocketThreadFunc( void* param )
{
#if ZP_USE_PROFILER
    Profiler::InitializeProfilerThread();
#endif

    AssetCompilerApplication* app = static_cast<AssetCompilerApplication*>(param);

    while( app->m_isRunning )
    {
        const Socket acceptedSocket = Platform::AcceptSocket( app->m_infoSocket );
        if( acceptedSocket && app->m_isRunning )
        {
            DataStreamWriter requestWriter( MemoryLabels::ThreadSafe, 4 KB );

            {
                FixedArray<zp_uint8_t, 1 KB> buffer;

                zp_size_t read;
                do
                {
                    read = Platform::ReceiveSocket( acceptedSocket, buffer.data(), buffer.length() );
                    Log::info() << "read: " << read << Log::endl;
                    if( read > 0 )
                    {
                        requestWriter.write( buffer.data(), read );
                    }
                } while( read > 0 );
            }

            const Memory requestMemory = requestWriter.memory();
            Log::info() << requestMemory.m_size << ": " << (const char*)requestMemory.as<char*>() << Log::endl;

            Http::Request request {};
            Http::ParseRequest( requestMemory, request );

            //
            //
            //

            DataStreamWriter responseWriter( MemoryLabels::ThreadSafe, 4 KB );
            const Http::Response response {
                .version = Http::Version::Http1_1,
                .statusCode = Http::StatusCode::OK,
                .body = String::As( "Hello HTTP!" ),
            };
            Http::WriteResponse( response, responseWriter );

            const Memory sendMemory = responseWriter.memory();

            Log::info() << sendMemory.m_size << ": " << (const char*)sendMemory.as<char*>() << Log::endl;

            Platform::SendSocket( acceptedSocket, sendMemory.m_ptr, sendMemory.m_size );

            Platform::CloseSocket( acceptedSocket );
        }
    }

#if ZP_USE_PROFILER
    Profiler::DestroyProfilerThread();
#endif

    return 0;
}

void AssetCompilerApplication::shutdown()
{
    ShaderCompiler::Shutdown();

    Platform::CloseSocket( m_infoSocket );

    Platform::JoinThreads( &m_receiveThread, 1 );
    Platform::CloseThread( m_receiveThread );

    Platform::CloseSystemTray( m_systemTray );

    Platform::ShutdownNetworking();

    JobSystem::ExitJobThreads();

    JobSystem::Teardown();

#if ZP_USE_PROFILER
    Profiler::DestroyProfilerThread();

    Profiler::DestroyProfiler();
#endif

    Log::info() << "Shutdown" << Log::endl;
}

void AssetCompilerApplication::process()
{
    if( m_isRunning )
    {
        m_isRunning = Platform::DispatchMessages( m_exitCode );
    }
}

[[nodiscard]] zp_bool_t AssetCompilerApplication::isRunning() const
{
    return m_isRunning;
}

[[nodiscard]] zp_int32_t AssetCompilerApplication::getExitCode() const
{
    return m_exitCode;
}

void AssetCompilerApplication::requestExit( zp_int32_t exitCode )
{
    m_isRunning = false;
    m_exitCode = exitCode;
}
