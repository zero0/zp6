#include "Core/Defines.h"
#include "Core/Common.h"
#include "Core/Allocator.h"
#include "Core/Queue.h"
#include "Core/CommandLine.h"
#include "Core/String.h"

#include "Platform/Platform.h"

#include "Tools/AssetCompiler.h"
#include "Tools/ShaderCompiler.h"

#include "EntryPoint/EntryPoint.h"


#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "Core/Job.h"
#include "Tools/AssetCompiler.h"

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

void CopyFileProcess( zp::AssetCompilerTask* task )
{
    zp::Platform::FileCopy( task->srcFile.c_str(), task->dstFile.c_str() );
}

struct MemoryConfig
{
    zp_size_t defaultAllocatorPageSize;
    zp_size_t tempAllocatorPageSize;
    zp_size_t profilerPageSize;
    zp_size_t debugPageSize;
};

void TestExec( zp::Job* job, zp::Memory memory )
{
    zp_printfln( "Exec Thead ID - %d", zp::Platform::GetCurrentThreadId() );
}

namespace zp
{


}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    EntryPointDesc desc;
    int ret = EntryPointMain<AssetCompilerApplication>( String::As( lpCmdLine ), desc );
    return ret;

#if 0
    using namespace zp;
    const zp_int32_t exitCode = EntryPointMain<int>();
    return exitCode;

    int exitCode = 0;

    using namespace zp;

    const zp_time_t startTime = Platform::TimeNow();

    MemoryConfig memoryConfig {
        .defaultAllocatorPageSize = 16 MB,
        .tempAllocatorPageSize = 2 MB,
        .profilerPageSize = 16 MB,
        .debugPageSize = 1 MB,
    };

#if ZP_DEBUG
    void* const baseAddress = reinterpret_cast<void*>(0x10000000);
#else
    void* const baseAddress = nullptr;
#endif

    const zp_size_t totalMemorySize = 128 MB;
    void* systemMemory = Platform::AllocateSystemMemory( baseAddress, totalMemorySize );

    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, CriticalSectionMemoryLock> s_defaultAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, 0 ), memoryConfig.defaultAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        CriticalSectionMemoryLock()
    );

    RegisterAllocator( 0, &s_defaultAllocator );

    CommandLine cmdLine( 0 );
    if( cmdLine.parse( lpCmdLine ) )
    {
        const CommandLineOperation versionOp = cmdLine.addOperation( {
            .longName = String::As( "--version" )
        } );
        const CommandLineOperation helpOp = cmdLine.addOperation( {
            .longName = String::As( "--help" )
        } );

        const CommandLineOperation inFileOp = cmdLine.addOperation( {
            .shortName = String::As( "-i" ),
            .longName = String::As( "--infile" ),
            .minParameterCount = 1,
            .maxParameterCount = 16
        } );

        const CommandLineOperation outFileOp = cmdLine.addOperation( {
            .shortName = String::As( "-o" ),
            .longName = String::As( "--outfile" ),
            .minParameterCount = 1,
            .maxParameterCount = 16
        } );

        const CommandLineOperation inDirOp = cmdLine.addOperation( {
            .shortName = String::As( "-id" ),
            .longName = String::As( "--indir" ),
            .minParameterCount = 1,
            .maxParameterCount = 16
        } );

        const CommandLineOperation outDirOp = cmdLine.addOperation( {
            .shortName = String::As( "-od" ),
            .longName = String::As( "--outdir" ),
            .minParameterCount = 1,
            .maxParameterCount = 1
        } );

        if( cmdLine.hasFlag( versionOp ) )
        {
            zp_printfln( "ZeroPoint6 AssetCompiler" );
        }
        else if( cmdLine.hasFlag( helpOp ) )
        {
            cmdLine.printHelp();
        }
        else
        {
            AssetCompiler assetCompiler( 0 );

            // register file extensions
            assetCompiler.registerFileExtension( String::As( ".shader" ), {
                .createTaskFunc =  ShaderCompiler::ShaderCompilerCreateTaskMemory,
                .deleteTaskFunc =  ShaderCompiler::ShaderCompilerDestroyTaskMemory,
                .executeFunc =     nullptr, //ShaderCompiler::ShaderCompilerExecute,
                .jobExecuteFunc =  ShaderCompiler::ShaderCompilerExecuteJob,
            } );
            assetCompiler.registerFileExtension( String::As( ".compute" ), {} );

            assetCompiler.registerFileExtension( String::As( ".tiff" ), {} );
            assetCompiler.registerFileExtension( String::As( ".png" ), {} );
            assetCompiler.registerFileExtension( String::As( ".jpg" ), {} );
            assetCompiler.registerFileExtension( String::As( ".jpeg" ), {} );

            // process input files/directories
            Vector<AllocString> inFiles( 16, 0 );
            Vector<AllocString> outFiles( 16, 0 );

            Queue<AssetCompilerTask> taskQueue( 16, 0 );

            if( cmdLine.hasFlag( inFileOp, true ) && cmdLine.hasFlag( outFileOp, true ) )
            {
                Vector<String> inFileParams( 8, 0 );
                Vector<String> outFileParams( 8, 0 );

                cmdLine.hasParameter( inFileOp, inFileParams );
                cmdLine.hasParameter( outFileOp, outFileParams );

                if( inFileParams.size() == outFileParams.size() )
                {
                    for( auto s : inFileParams )
                    {
                        inFiles.pushBack( AllocString( 0, s.str, s.length ) );
                    }
                    for( auto s : outFileParams )
                    {
                        outFiles.pushBack( AllocString( 0, s.str, s.length ) );
                    }
                }
                else
                {

                }
            }
            else if( cmdLine.hasFlag( inDirOp, true ) && cmdLine.hasFlag( outDirOp, true ) )
            {

            }
            else
            {
                cmdLine.printHelp();
            }

            // convert files to tasks
            if( !inFiles.isEmpty() && !outFiles.isEmpty() && inFiles.size() == outFiles.size() )
            {
                for( zp_size_t i = 0; i < inFiles.size(); ++i )
                {
                    const AllocString& inFile = inFiles[ i ];
                    const AllocString& outFile = outFiles[ i ];
                    String ext = String::As( zp_strrchr( inFile.c_str(), '.' ) );

                    const AssetCompilerProcessor* assetCompilerProcessor = assetCompiler.getCompilerProcessor( ext );
                    if( assetCompilerProcessor )
                    {
                        Memory taskMemory {};
                        if( assetCompilerProcessor->createTaskFunc )
                        {
                            taskMemory = assetCompilerProcessor->createTaskFunc( 0, inFile.asString(), outFile.asString(), cmdLine );
                        }

                        taskQueue.enqueue( {
                            .srcFile = inFile,
                            .dstFile = outFile,
                            .exec = assetCompilerProcessor->executeFunc,
                            .jobExec = assetCompilerProcessor->jobExecuteFunc,
                            .deleteTaskMemory = assetCompilerProcessor->deleteTaskFunc,
                            .taskMemory = taskMemory,
                            .memoryLabel = 0,
                        } );
                    }
                }
            }

            Vector<AssetCompilerTask> tasksToDelete( 16, 0 );

            JobSystem::Setup( 0, 2 );

            JobSystem::InitializeJobThreads();

            auto parent = JobSystem::Start().schedule();
            const bool useJobSystem = false;

            // execute tasks
            while( !taskQueue.isEmpty() )
            {
                AssetCompilerTask task = taskQueue.dequeue();

                if( useJobSystem )
                {
                    JobSystem::Start( task, parent ).schedule();
                }
                else
                {
                    AssetCompilerTask::Execute( JobHandle::Null, &task );
                }


                tasksToDelete.pushBack( task );
            }

            JobSystem::ScheduleBatchJobs();
            parent.complete();

            for( const AssetCompilerTask& task : tasksToDelete )
            {
                if( task.deleteTaskMemory )
                {
                    task.deleteTaskMemory( task.memoryLabel, task.taskMemory );
                }
            }

            JobSystem::ExitJobThreads();

            JobSystem::Teardown();
        }
    }
    else
    {
        cmdLine.printHelp();
    }

    const zp_time_t endTime = Platform::TimeNow();
    zp_printfln( "%s Duration: %fs", exitCode == 0 ? "Success" : "Error", zp_float32_t( endTime - startTime ) / zp_float32_t( Platform::TimeFrequency() ) );

    return exitCode;
#endif
}
