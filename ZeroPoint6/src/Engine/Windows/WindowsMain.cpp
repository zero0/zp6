
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"
#include "Engine/Engine.h"

struct MemoryConfig
{
    zp_size_t defaultAllocatorPageSize;
    zp_size_t tempAllocatorPageSize;
    zp_size_t threadSafeAllocatorPageSize;
    zp_size_t profilerPageSize;
    zp_size_t debugPageSize;
};

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    using namespace zp;

    MemoryConfig memoryConfig {
        .defaultAllocatorPageSize = 16 MB,
        .tempAllocatorPageSize = 2 MB,
        .threadSafeAllocatorPageSize = 2 MB,
        .profilerPageSize = 16 MB,
        .debugPageSize = 1 MB,
    };

#if ZP_DEBUG
    void* const baseAddress = reinterpret_cast<void*>(0x10000000);
#else
    void* const baseAddress = nullptr;
#endif

    const zp_size_t totalMemorySize = 256 MB;
    void* systemMemory = Platform::AllocateSystemMemory( baseAddress, totalMemorySize );

    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_defaultAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, 0 ), memoryConfig.defaultAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );

    zp_size_t endMemorySize = totalMemorySize;

    const zp_size_t tempMemorySize = 16 MB;
    endMemorySize -= tempMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_tempAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.tempAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );

    const zp_size_t threadSafeMemorySize = 8 MB;
    endMemorySize -= threadSafeMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, CriticalSectionMemoryLock> s_threadSafeAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.threadSafeAllocatorPageSize ),
        TlsfAllocatorPolicy(),
        CriticalSectionMemoryLock()
    );

#if ZP_USE_PROFILER
    const zp_size_t profilerMemorySize = 8 MB;
    endMemorySize -= profilerMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_profilingAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.profilerPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );
#else
    MemoryAllocator<NullMemoryStorage, NullAllocationPolicy, NullMemoryLock> s_profilingAllocator(
        NullMemoryStorage,
        NullAllocationPolicy,
        NullMemoryLock
    );
#endif

#if ZP_DEBUG
    const zp_size_t debugMemorySize = 8 MB;
    endMemorySize -= debugMemorySize;
    MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_debugAllocator(
        SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), memoryConfig.debugPageSize ),
        TlsfAllocatorPolicy(),
        NullMemoryLock()
    );
#else
    MemoryAllocator<NullMemoryStorage, NullAllocationPolicy, NullMemoryLock> s_debugAllocator(
        NullMemoryStorage,
        NullAllocationPolicy,
        NullMemoryLock
    );
#endif

    RegisterAllocator( MemoryLabels::Default, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::String, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Graphics, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::FileIO, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Buffer, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::User, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Data, &s_defaultAllocator );
    RegisterAllocator( MemoryLabels::Temp, &s_tempAllocator );
    RegisterAllocator( MemoryLabels::ThreadSafe, &s_threadSafeAllocator );
    RegisterAllocator( MemoryLabels::Profiling, &s_profilingAllocator );
    RegisterAllocator( MemoryLabels::Debug, &s_debugAllocator );

    auto engine = ZP_NEW( Default, Engine );
    {
        engine->processCommandLine( String::As( lpCmdLine ) );

        engine->initialize();

        engine->startEngine();

        do
        {
            engine->process();
        } while( engine->isRunning() );

        engine->stopEngine();

        engine->destroy();
    }

    const zp_int32_t exitCode = engine->getExitCode();

    ZP_FREE( Default, engine );

    return exitCode;
}
