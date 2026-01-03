//
// Created by phosg on 12/5/2023.
//

#ifndef ZP_ENTRYPOINT_H
#define ZP_ENTRYPOINT_H

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/Properties.h"
#include "Core/String.h"
#include "Core/Profiler.h"
#include "Core/Job.h"

#include "Platform/Platform.h"

#include "Engine/MemoryLabels.h"

#include "Test/Test.h"

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
        zp_size_t totalMemorySize = 512 MB;
        MemoryConfig defaultAllocator =     { .totalSize = 0 MB,    .pageSize = 16 MB };
        MemoryConfig tempAllocator =        { .totalSize = 32 MB,   .pageSize = 2 MB };
        MemoryConfig threadSafeAllocator =  { .totalSize = 16 MB,   .pageSize = 2 MB };
        MemoryConfig profilerAllocator =    { .totalSize = 64 MB,   .pageSize = 0 };
        MemoryConfig debugAllocator =       { .totalSize = 16 MB,   .pageSize = 2 MB };
        MemoryConfig graphicsAllocator =    { .totalSize = 0 MB,    .pageSize = 4 MB };
    };
    // @formatter:on

    template<typename T>
    ZP_FORCEINLINE zp_int32_t EntryPointMain( const String& commandLine, const EntryPointDesc& desc )
    {
        EntryPointDesc entryPointDesc = desc;

        zp_bool_t disableNetworking = false;
        zp_int32_t maxJobThreads = 2;

        // read boot config file
        if( Platform::FileExists( "boot.config" ) )
        {
            auto bootConfigFile = Platform::OpenFileHandle( "boot.config", ZP_OPEN_FILE_MODE_READ, ZP_CREATE_FILE_MODE_OPEN );
            const zp_size_t fileSize = Platform::GetFileSize( bootConfigFile );

            if( fileSize > 0 )
            {
                const AllocMemory buffer( MemoryLabels::Default, fileSize );
                Platform::ReadFile( bootConfigFile, buffer.ptr, buffer.size );

                Platform::CloseFileHandle( bootConfigFile );

                Properties properties( MemoryLabels::Default );
                properties.TryParse( String::As( buffer.memory() ) );

                properties.TryGetPropertyAsSizeT( String::As( "total-memory-size" ), entryPointDesc.totalMemorySize );
                properties.TryGetPropertyAsBoolean( String::As( "disable-networking" ), disableNetworking );
                properties.TryGetPropertyAsInt32( String::As( "max-job-threads" ), maxJobThreads );
            }
        }

#if ZP_DEBUG
        void* const baseAddress = reinterpret_cast<void*>(0x10000000);
#else
        void* const baseAddress = nullptr;
#endif

        // allocate all memory
        void* systemMemory = Platform::AllocateSystemMemory( baseAddress, entryPointDesc.totalMemorySize );

        // default allocator
        MemoryAllocator s_defaultAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, 0 ), entryPointDesc.defaultAllocator.pageSize, entryPointDesc.defaultAllocator.totalSize ),
            TlsfAllocatorPolicy(),
            NullMemoryLock(),
            TrackedMemoryProfiler()
            );

        // use as offset for other memory pools
        zp_size_t endMemorySize = entryPointDesc.totalMemorySize;

        endMemorySize -= entryPointDesc.tempAllocator.totalSize;
        MemoryAllocator s_tempAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), entryPointDesc.tempAllocator.pageSize, entryPointDesc.tempAllocator.totalSize ),
            TlsfAllocatorPolicy(),
            NullMemoryLock(),
            TrackedMemoryProfiler()
            );

        endMemorySize -= entryPointDesc.threadSafeAllocator.totalSize;
        MemoryAllocator s_threadSafeAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), entryPointDesc.threadSafeAllocator.pageSize, entryPointDesc.threadSafeAllocator.totalSize ),
            TlsfAllocatorPolicy(),
            CriticalSectionMemoryLock(),
            TrackedMemoryProfiler()
            );

#if ZP_USE_PROFILER
        endMemorySize -= entryPointDesc.profilerAllocator.totalSize;
        MemoryAllocator s_profilingAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), entryPointDesc.profilerAllocator.pageSize, entryPointDesc.profilerAllocator.totalSize ),
            TlsfAllocatorPolicy(),
            CriticalSectionMemoryLock(),
            NullMemoryProfiler()
            );
#else
        MemoryAllocator s_profilingAllocator(
            NullMemoryStorage,
            NullAllocationPolicy,
            NullMemoryLock,
            NullMemoryProfiler );
#endif

#if ZP_DEBUG
        endMemorySize -= entryPointDesc.debugAllocator.totalSize;
        MemoryAllocator s_debugAllocator(
            SystemPageMemoryStorage( ZP_OFFSET_PTR( systemMemory, endMemorySize ), entryPointDesc.debugAllocator.pageSize, entryPointDesc.defaultAllocator.totalSize ),
            TlsfAllocatorPolicy(),
            NullMemoryLock(),
            TrackedMemoryProfiler()
            );
#else
        MemoryAllocator s_debugAllocator(
            NullMemoryStorage,
            NullAllocationPolicy,
            NullMemoryLock,
            NullMemoryProfiler );
#endif

        // @formatter:off
        // register allocators
        RegisterAllocator( MemoryLabels::Default,       &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::String,        &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::Graphics,      &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::FileIO,        &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::Buffer,        &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::User,          &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::Data,          &s_defaultAllocator );
        RegisterAllocator( MemoryLabels::Temp,          &s_tempAllocator );
        RegisterAllocator( MemoryLabels::ThreadSafe,    &s_threadSafeAllocator );

        RegisterAllocator( MemoryLabels::Profiling,     &s_profilingAllocator );
        RegisterAllocator( MemoryLabels::Debug,         &s_debugAllocator );
        // @formatter:on

        // initialize networking (if needed)
        if( !disableNetworking )
        {
            Platform::InitializeNetworking();
        }

        // initialize stack trace
        Platform::InitializeStackTrace();

        // calculate max job threads
        if( maxJobThreads == 0 )
        {
            maxJobThreads = static_cast<zp_int32_t>(Platform::GetProcessorCount()) - 1;
        }

        const zp_int32_t numJobThreads = zp_min( maxJobThreads, static_cast<zp_int32_t>(Platform::GetProcessorCount()) - 1 );

        // set main thread
        const ThreadHandle mainThreadHandle = Platform::GetCurrentThread();
        Platform::SetThreadName( mainThreadHandle, String::As( "MainThread" ) );
        Platform::SetThreadIdealProcessor( mainThreadHandle, 0 );

#if ZP_USE_PROFILER
        Profiler::CreateProfiler( MemoryLabels::Profiling, {
            .maxCPUEventsPerThread = 128,
            .maxMemoryEventsPerThread = 128,
            .maxGPUEventsPerThread = 4,
            .maxFramesToCapture = 120,
            .maxThreadCount = static_cast<zp_uint32_t>(numJobThreads) + 1,
        } );

        Profiler::InitializeProfilerThread();
#endif

        // initialize job system
        JobSystem::Setup( MemoryLabels::Default, numJobThreads );

        JobSystem::InitializeJobThreads();

        //
        //
        //

        zp_int32_t exitCode = 0;

#if ZP_USE_TESTS
        // run all tests
        {
            TestResults results( MemoryLabels::Default );
            TestRunner::RunAll( results );
        }
#endif

        // run app
        {
            T* app = ZP_NEW( MemoryLabels::Default, T );

            app->initialize( commandLine );

            do
            {
                app->process();
            } while( app->isRunning() );

            app->shutdown();

            exitCode = app->getExitCode();

            ZP_SAFE_DELETE( T, app );
        }

#if ZP_USE_PROFILER
        Profiler::DestroyProfilerThread();

        Profiler::DestroyProfiler();
#endif

        JobSystem::Teardown();

        Platform::FreeSystemMemory( systemMemory );

        Platform::ShutdownStackTrace();

        if( !disableNetworking )
        {
            Platform::ShutdownNetworking();
        }

        return exitCode;
    }
}

#endif //ZP_ENTRYPOINT_H
