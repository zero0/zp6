//
// Created by phosg on 7/16/2023.
//

#include "Core/CommandLine.h"
#include "Core/Log.h"

#include "Tools/AssetCompiler.h"
#include "Tools/ShaderCompiler.h"

using namespace zp;


void zp::AssetCompilerTask::Execute( const zp::JobHandle& jobHandle, zp::AssetCompilerTask* task )
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
    , m_isRunning( true )
    , m_infoPort( 8080 )
    , m_assetPort( 40000 )
    , memoryLabel( memoryLabel )
{
}

void AssetCompilerApplication::initialize()
{
    Platform::InitializeNetworking();

    m_systemTray = Platform::OpenSystemTray( {} );

    Log::info() << "Register File Extensions..." << Log::endl;

    // register file extensions
    m_assetCompiler.registerFileExtension( String::As( ".shader" ), {
        .createTaskFunc =  ShaderCompiler::ShaderCompilerCreateTaskMemory,
        .deleteTaskFunc =  ShaderCompiler::ShaderCompilerDestroyTaskMemory,
        .executeFunc =     nullptr, //ShaderCompiler::ShaderCompilerExecute,
        .jobExecuteFunc =  ShaderCompiler::ShaderCompilerExecuteJob,
    } );
    m_assetCompiler.registerFileExtension( String::As( ".compute" ), {} );

    m_assetCompiler.registerFileExtension( String::As( ".tiff" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".png" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".jpg" ), {} );
    m_assetCompiler.registerFileExtension( String::As( ".jpeg" ), {} );

    Log::info() << "Asset Port: " << m_assetPort << Log::endl;

    m_infoSocket = Platform::OpenSocket( {
        .name = "AssetCompiler::InfoSocket",
        .address = IPAddress::Localhost( m_infoPort ),
        .addressFamily = AddressFamily::IPv4,
        .socketType = SocketType::Stream,
        .connectionProtocol = ConnectionProtocol::TCP,
        .socketDirection = SocketDirection::Listen,
    } );
    Log::info() << "Info Listening on Port:  " << m_infoPort << Log::endl;

    m_receiveThread = Platform::CreateThread( ReceiveInfoSocketThreadFunc, this, 1 MB, nullptr );
}

zp_uint32_t AssetCompilerApplication::ReceiveInfoSocketThreadFunc( void* param )
{
    AssetCompilerApplication* app = static_cast<AssetCompilerApplication*>(param);

    while( app->m_isRunning )
    {
        Socket acceptedSocket = Platform::AcceptSocket( app->m_infoSocket );
        if( acceptedSocket && app->m_isRunning )
        {
            FixedArray<zp_uint8_t, 8 KB> mem;
            zp_size_t read = Platform::ReceiveSocket( acceptedSocket, mem.data(), mem.length() );

            zp_printfln( "%d: %.*s", read, read, mem.data() );

            //Platform::SendSocket( acceptedSocket, mem.data(), read );

            Platform::CloseSocket( acceptedSocket );
        }
    }

    return 0;
}

void AssetCompilerApplication::shutdown()
{
    Platform::CloseSocket( m_infoSocket );

    Platform::JoinThreads( &m_receiveThread, 1 );
    Platform::CloseThread( m_receiveThread );

    Platform::CloseSystemTray( m_systemTray );

    Platform::ShutdownNetworking();

    Log::info() << "Shutdown" << Log::endl;
}

void AssetCompilerApplication::processCommandLine( const String& commandLine )
{
    CommandLine cmdLine( 0 );
    if( cmdLine.parse( commandLine.c_str() ) )
    {
        const CommandLineOperation versionOp = cmdLine.addOperation( {
            .longName = String::As( "--version" )
        } );
        const CommandLineOperation helpOp = cmdLine.addOperation( {
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

        //
        // Operations
        //

        if( cmdLine.hasFlag( versionOp ) )
        {
            Log::message() << "Version: " << "???" << Log::endl;
        }

        if( cmdLine.hasFlag( helpOp ) )
        {
            cmdLine.printHelp();
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
    }
    else
    {
        ZP_ASSERT_MSG_ARGS( false, "Failed to parse command line: %s", commandLine.c_str() );
    }
}

void AssetCompilerApplication::process()
{
    m_isRunning = Platform::DispatchMessages( m_exitCode );
}

[[nodiscard]] zp_bool_t AssetCompilerApplication::isRunning() const
{
    return m_isRunning;
}

[[nodiscard]] zp_int32_t AssetCompilerApplication::getExitCode() const
{
    return m_exitCode;
}
