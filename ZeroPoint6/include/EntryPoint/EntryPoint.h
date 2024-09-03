//
// Created by phosg on 12/5/2023.
//

#ifndef ZP_ENTRYPOINT_H
#define ZP_ENTRYPOINT_H

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"

namespace zp
{
    ZP_PURE_INTERFACE IEngine
    {
    public:
        virtual void ProcessCommandLine( const String& cmdLine ) = 0;

        virtual void Process() = 0;

        [[nodiscard]] virtual zp_bool_t IsRunning() const = 0;

        [[nodiscard]] virtual zp_int32_t GetExitCode() const = 0;
    };

    struct MemoryConfig
    {
        zp_size_t totalSize;
        zp_size_t pageSize;
    };

    // @formatter:off
    struct EntryPointDesc
    {
        zp_size_t totalMemorySize = 256 MB;
        MemoryConfig defaultAllocator =     { .totalSize = 0 MB,    .pageSize = 16 MB };
        MemoryConfig tempAllocator =        { .totalSize = 16 MB,   .pageSize = 2 MB };
        MemoryConfig threadSafeAllocator =  { .totalSize = 8 MB,    .pageSize = 2 MB };
        MemoryConfig profilerAllocator =    { .totalSize = 16 MB,   .pageSize = 2 MB };
        MemoryConfig debugAllocator =       { .totalSize = 8 MB,    .pageSize = 2 MB };
        MemoryConfig graphicsAllocator =    { .totalSize = 0 MB,    .pageSize = 4 MB };
    };
    // @formatter:on

    template<typename T>
    ZP_FORCEINLINE zp_int32_t EntryPointMain( const String& commandLine, const EntryPointDesc& desc )
    {
#if ZP_DEBUG
        void* const baseAddress = reinterpret_cast<void*>(0x10000000);
#else
        void* const baseAddress = nullptr;
#endif

        // allocate all memory
        const zp_size_t totalMemorySize = desc.totalMemorySize;
        void* systemMemory = Platform::AllocateSystemMemory( baseAddress, totalMemorySize );

        // default allocator
        MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_defaultAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, 0 ), desc.defaultAllocator.pageSize ),
            TlsfAllocatorPolicy(),
            NullMemoryLock()
        );

        // use as offset for other memory pools
        zp_size_t endMemorySize = totalMemorySize;

        endMemorySize -= desc.tempAllocator.totalSize;
        MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_tempAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), desc.tempAllocator.pageSize ),
            TlsfAllocatorPolicy(),
            NullMemoryLock()
        );

        endMemorySize -= desc.threadSafeAllocator.totalSize;
        MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, CriticalSectionMemoryLock> s_threadSafeAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), desc.threadSafeAllocator.pageSize ),
            TlsfAllocatorPolicy(),
            CriticalSectionMemoryLock()
        );

#if ZP_USE_PROFILER
        endMemorySize -= desc.profilerAllocator.totalSize;
        MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_profilingAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), desc.profilerAllocator.pageSize ),
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
        endMemorySize -= desc.debugAllocator.totalSize;
        MemoryAllocator<SystemPageMemoryStorage, TlsfAllocatorPolicy, NullMemoryLock> s_debugAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), desc.debugAllocator.pageSize ),
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

        // register allocators
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

        // run app
        T* app = ZP_NEW( MemoryLabels::Default, T );

        app->processCommandLine( commandLine );

        app->initialize();

        do
        {
            app->process();
        } while( app->isRunning() );

        app->shutdown();

        const zp_int32_t exitCode = app->getExitCode();

        ZP_SAFE_DELETE( T, app );

        return exitCode;
    }
}

#endif //ZP_ENTRYPOINT_H
