
#include "Core/Defines.h"
#include "Core/Common.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/CommandLine.h"

#include "Platform/Platform.h"

#include "Tools/AssetCompiler.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct MemoryConfig
{
    zp_size_t defaultAllocatorPageSize;
    zp_size_t tempAllocatorPageSize;
    zp_size_t profilerPageSize;
    zp_size_t debugPageSize;
};

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    int exitCode = 0;

    using namespace zp;

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
    void* systemMemory = GetPlatform()->AllocateSystemMemory( baseAddress, totalMemorySize );

    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_defaultAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, 0 ), memoryConfig.defaultAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );

    RegisterAllocator( 0, &s_defaultAllocator );

    CommandLine cmdLine( 0 );
    if( cmdLine.parse( lpCmdLine ) )
    {
        const CommandLineOperation versionOp = cmdLine.addOperation( { .longName = "--version" } );
        const CommandLineOperation helpOp = cmdLine.addOperation( { .longName = "--help" } );

        const CommandLineOperation outFileOp = cmdLine.addOperation( { .shortName = "-o", .longName = "--outfile", .minParameterCount  = 1, .maxParameterCount = 1 } );

        if( cmdLine.hasFlag( versionOp ) )
        {

        }

        if( cmdLine.hasFlag( helpOp ) )
        {
            cmdLine.printHelp();
        }
    }
    else
    {
        cmdLine.printHelp();
    }

    InitDxcCompiler();

    return exitCode;
}
