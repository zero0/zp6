//
// Created by phosg on 1/27/2022.
//

#include "Core/Defines.h"

#if ZP_USE_PROFILER

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Atomic.h"
#include "Core/Profiler.h"
#include "Core/Memory.h"
#include "Core/Allocator.h"

#include "Platform/Platform.h"

namespace zp
{
    namespace
    {

        struct CPUProfilerEvent
        {
            const char* filename;
            const char* functionName;
            const char* eventName;
            zp_uint64_t frameIndex;
            zp_uint64_t startCycle;
            zp_uint64_t endCycle;
            zp_time_t startTime;
            zp_time_t endTime;
            zp_ptr_t userData;
            zp_uint32_t parentEvent;
            zp_uint32_t lineNumber;
            zp_uint32_t threadId;
            StackTrace stackTrace;
        };

        struct MemoryProfilerEvent
        {
            zp_uint64_t frameIndex;
            zp_uint64_t cycle;
            zp_time_t time;
            zp_ptr_t memoryAddress;
            zp_uint64_t memoryAllocated;
            zp_uint64_t memoryFreed;
            zp_uint64_t memoryTotal;
            zp_uint64_t memoryCapacity;
            zp_ptr_t userData;
            zp_uint32_t memoryLabel;
            zp_uint32_t threadId;
            StackTrace stackTrace;
        };

        struct GPUProfilerEvent
        {
            zp_uint64_t frameIndex;

            zp_time_t gpuDuration;
            zp_time_t time;

            zp_uint32_t numDrawCalls;
            zp_uint32_t numTriangles;
            zp_uint32_t numCommands;

            zp_uint32_t numRenderTargets;
            zp_uint32_t numTextures;
            zp_uint32_t numShaders;
            zp_uint32_t numMaterials;
            zp_uint32_t numMeshes;

            zp_uint32_t threadId;
        };
    }

    namespace
    {
        enum
        {
            kMaxEventStackCount = 32
        };

        struct ProfilerThreadData
        {
            zp_uint64_t currentFrame;

            MemoryArray<CPUProfilerEvent> cpuProfilerData;
            MemoryArray<MemoryProfilerEvent> memProfilerData;
            MemoryArray<GPUProfilerEvent> gpuProfilerData;

            zp_size_t currentCPUProfilerEvent;
            zp_size_t currentMemoryProfilerEvent;
            zp_size_t currentGPUProfilerEvent;

            zp_size_t eventStackCount;
            zp_size_t eventStack[kMaxEventStackCount];

            zp_size_t threadIndex;
            zp_uint32_t threadID;
        };

        thread_local ProfilerThreadData t_profilerData;
    }

    namespace
    {
        struct ProfilerContext
        {
            zp_uint64_t currentFrame;

            zp_size_t framesToCapture;
            zp_size_t maxProfilerThreads;
            zp_size_t maxCpuEventsPerThread;
            zp_size_t maxGpuEventsPerThread;
            zp_size_t maxMemoryEventsPerThread;

            MemoryArray<CPUProfilerEvent> cpuProfilerData;
            MemoryArray<MemoryProfilerEvent> memProfilerData;
            MemoryArray<GPUProfilerEvent> gpuProfilerData;

            zp_size_t profilerThreadDataCount;
            MemoryArray<ProfilerThreadData*> profilerThreadData;

            MemoryLabel memoryLabel;
        };

        ProfilerContext g_context;
    }

    namespace
    {
        void AdvanceProfilerFrame( zp_uint64_t frameIndex )
        {
            g_context.currentFrame = frameIndex;
            for( zp_size_t i = 0; i < g_context.profilerThreadDataCount; ++i )
            {
                ProfilerThreadData* threadData = g_context.profilerThreadData[ i ];
                if( threadData != nullptr )
                {
                    Atomic::Exchange( &threadData->currentFrame, frameIndex );
                }
            }
        }

        void RegisterProfilerThread( ProfilerThreadData* profilerThreadData )
        {
            const zp_size_t profilerThreadIndex = Atomic::IncrementSizeT( &g_context.profilerThreadDataCount ) - 1;

            profilerThreadData->currentCPUProfilerEvent = 0;
            profilerThreadData->currentGPUProfilerEvent = 0;
            profilerThreadData->currentMemoryProfilerEvent = 0;

            profilerThreadData->cpuProfilerData = g_context.cpuProfilerData.split( profilerThreadIndex * g_context.maxCpuEventsPerThread, g_context.maxCpuEventsPerThread );
            profilerThreadData->gpuProfilerData = g_context.gpuProfilerData.split( profilerThreadIndex * g_context.maxGpuEventsPerThread, g_context.maxGpuEventsPerThread );
            profilerThreadData->memProfilerData = g_context.memProfilerData.split( profilerThreadIndex * g_context.maxMemoryEventsPerThread, g_context.maxMemoryEventsPerThread );

            profilerThreadData->eventStackCount = 0;

            profilerThreadData->threadIndex = profilerThreadIndex;
            profilerThreadData->threadID = Platform::GetCurrentThreadId();

            g_context.profilerThreadData[ profilerThreadIndex ] = profilerThreadData;
        }

        void UnregisterProfilerThread( ProfilerThreadData* profilerThreadData )
        {
            g_context.profilerThreadData[ profilerThreadData->threadIndex ] = {};
        }
    }

    namespace
    {
        zp_int32_t compareCPU( const CPUProfilerEvent& lh, const CPUProfilerEvent& rh )
        {
            zp_int32_t cmp = zp_cmp( lh.threadId, rh.threadId );
            return cmp;// == 0 ? -zp_cmp( lh.startTime, rh.startTime ) : cmp;
        }

        zp_int32_t compareMemory( const MemoryProfilerEvent& lh, const MemoryProfilerEvent& rh )
        {
            zp_int32_t cmp = zp_cmp( lh.threadId, rh.threadId );
            return cmp == 0 ? zp_cmp( lh.time, rh.time ) : cmp;
        }

        zp_int32_t compareGPU( const GPUProfilerEvent& lh, const GPUProfilerEvent& rh )
        {
            zp_int32_t cmp = zp_cmp( lh.threadId, rh.threadId );
            return cmp == 0 ? zp_cmp( lh.time, rh.time ) : cmp;
        }
    }

    //
    //
    //

    void Profiler::CreateProfiler( MemoryLabel memoryLabel, const ProfilerCreateDesc& profilerCreateDesc )
    {
        const zp_size_t cpuEventCount = ( profilerCreateDesc.maxFramesToCapture * profilerCreateDesc.maxThreadCount * profilerCreateDesc.maxCPUEventsPerThread );
        const zp_size_t memEventCount = ( profilerCreateDesc.maxFramesToCapture * profilerCreateDesc.maxThreadCount * profilerCreateDesc.maxMemoryEventsPerThread );
        const zp_size_t gpuEventCount = ( profilerCreateDesc.maxFramesToCapture * profilerCreateDesc.maxThreadCount * profilerCreateDesc.maxGPUEventsPerThread );

        g_context = {
            .currentFrame = 0,
            .framesToCapture = profilerCreateDesc.maxFramesToCapture,
            .maxProfilerThreads = profilerCreateDesc.maxThreadCount,
            .maxCpuEventsPerThread = profilerCreateDesc.maxCPUEventsPerThread,
            .maxGpuEventsPerThread =  profilerCreateDesc.maxGPUEventsPerThread,
            .maxMemoryEventsPerThread = profilerCreateDesc.maxMemoryEventsPerThread,
            .cpuProfilerData = { ZP_MALLOC_T_ARRAY( memoryLabel, CPUProfilerEvent, cpuEventCount ), cpuEventCount },
            .memProfilerData = { ZP_MALLOC_T_ARRAY( memoryLabel, MemoryProfilerEvent, memEventCount ), memEventCount },
            .gpuProfilerData = { ZP_MALLOC_T_ARRAY( memoryLabel, GPUProfilerEvent, gpuEventCount ), gpuEventCount },
            .profilerThreadDataCount = 0,
            .profilerThreadData = { ZP_MALLOC_T_ARRAY( memoryLabel, ProfilerThreadData*, profilerCreateDesc.maxThreadCount ), profilerCreateDesc.maxThreadCount },
            .memoryLabel = memoryLabel
        };
#if 0
        // thread scratch buffers
        const zp_size_t cpuProfileEventSize = sizeof( CPUProfilerEvent ) * m_maxCPUProfilerEvents;
        const zp_size_t memoryProfileEventSize = sizeof( MemoryProfilerEvent ) * m_maxMemoryProfilerEvents;
        const zp_size_t gpuProfileEventSize = sizeof( GPUProfilerEvent ) * m_maxGPUProfilerEvents;

        const zp_size_t cpuProfilerEventAllocationSize = cpuProfileEventSize * maxProfilerThreads;
        const zp_size_t memoryProfilerEventAllocationSize = memoryProfileEventSize * maxProfilerThreads;
        const zp_size_t gpuProfilerEventAllocationSize = gpuProfileEventSize * maxProfilerThreads;

        m_cpuProfilerData = ZP_MALLOC_T_ARRAY( memoryLabel, CPUProfilerEvent, cpuProfilerEventAllocationSize );
        m_memoryProfilerData = ZP_MALLOC_T_ARRAY( memoryLabel, MemoryProfilerEvent, memoryProfilerEventAllocationSize );
        m_gpuProfilerData = ZP_MALLOC_T_ARRAY( memoryLabel, GPUProfilerEvent, gpuProfilerEventAllocationSize );

        zp_zero_memory( m_cpuProfilerData, cpuProfilerEventAllocationSize );
        zp_zero_memory( m_memoryProfilerData, memoryProfilerEventAllocationSize );
        zp_zero_memory( m_gpuProfilerData, gpuProfilerEventAllocationSize );

        // thread profiler data
        profilerThreadData = ZP_MALLOC_T_ARRAY( memoryLabel, ProfilerThreadData*, m_profilerDataThreadCapacity );

        // profiler frame data
        m_profilerFrameStride = ( sizeof( ProfilerFrameHeader ) + cpuProfileEventSize + memoryProfileEventSize + gpuProfileEventSize );
        const zp_size_t profilerFrameBufferAllocationSize = m_profilerFrameStride * framesToCapture;

        m_profilerFrameBuffers = ZP_MALLOC( memoryLabel, profilerFrameBufferAllocationSize );

        zp_zero_memory( m_profilerFrameBuffers, profilerFrameBufferAllocationSize );
#endif
    }

    void Profiler::DestroyProfiler()
    {
        ZP_FREE( g_context.memoryLabel, g_context.cpuProfilerData.data() );
        ZP_FREE( g_context.memoryLabel, g_context.memProfilerData.data() );
        ZP_FREE( g_context.memoryLabel, g_context.gpuProfilerData.data() );
        ZP_FREE( g_context.memoryLabel, g_context.profilerThreadData.data() );

        g_context = {};
#if 0
        ZP_FREE( memoryLabel, m_cpuProfilerData );
        ZP_FREE( memoryLabel, m_memoryProfilerData );
        ZP_FREE( memoryLabel, m_gpuProfilerData );

        ZP_FREE( memoryLabel, profilerThreadData );

        ZP_FREE( memoryLabel, m_profilerFrameBuffers );

        g_profiler = nullptr;
#endif
    }

    void Profiler::InitializeProfilerThread()
    {
        t_profilerData = {};

        RegisterProfilerThread( &t_profilerData );
    }

    void Profiler::DestroyProfilerThread()
    {
        UnregisterProfilerThread( &t_profilerData );

        t_profilerData = {};
    }

    void Profiler::MarkCPU( const CPUDesc& cpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentCPUProfilerEvent++ % t_profilerData.cpuProfilerData.length();

        CPUProfilerEvent* event = t_profilerData.cpuProfilerData.data() + eventIndex;
        event->filename = cpuDesc.filename;
        event->functionName = cpuDesc.functionName;
        event->eventName = cpuDesc.eventName;
        event->frameIndex = t_profilerData.currentFrame;
        event->startCycle = Platform::TimeCycles();
        event->endCycle = event->startCycle;
        event->startTime = Platform::TimeNow();
        event->endTime = event->startTime;
        event->parentEvent = t_profilerData.eventStackCount > 0 ? t_profilerData.eventStack[ t_profilerData.eventStackCount - 1 ] : 0;
        event->userData = cpuDesc.userData;
        event->lineNumber = cpuDesc.lineNumber;
        event->threadId = t_profilerData.threadID;
        Platform::GetStackTrace( event->stackTrace );
    }

    zp_size_t Profiler::StartCPU( const CPUDesc& cpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentCPUProfilerEvent++ % t_profilerData.cpuProfilerData.length();

        CPUProfilerEvent* event = t_profilerData.cpuProfilerData.data() + eventIndex;
        event->filename = cpuDesc.filename;
        event->functionName = cpuDesc.functionName;
        event->eventName = cpuDesc.eventName;
        event->frameIndex = t_profilerData.currentFrame;
        event->startCycle = Platform::TimeCycles();
        event->endCycle = event->startCycle;
        event->startTime = Platform::TimeNow();
        event->endTime = event->startTime;
        event->parentEvent = t_profilerData.eventStackCount > 0 ? t_profilerData.eventStack[ t_profilerData.eventStackCount - 1 ] : -1;
        event->userData = cpuDesc.userData;
        event->lineNumber = cpuDesc.lineNumber;
        event->threadId = t_profilerData.threadID;
        Platform::GetStackTrace( event->stackTrace );

        ZP_ASSERT( t_profilerData.eventStackCount != kMaxEventStackCount );
        t_profilerData.eventStack[ t_profilerData.eventStackCount++ ] = eventIndex;

        return eventIndex;
    }

    void Profiler::EndCPU( zp_size_t eventIndex )
    {
        CPUProfilerEvent* event = t_profilerData.cpuProfilerData.data() + eventIndex;

        event->endTime = Platform::TimeNow();
        event->endCycle = Platform::TimeCycles();

        ZP_ASSERT( t_profilerData.eventStackCount > 0 );
        if( t_profilerData.eventStackCount > 0 )
        {
            --t_profilerData.eventStackCount;
        }
    }

    void Profiler::MarkMemory( const Profiler::MemoryDesc& memoryDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentMemoryProfilerEvent++ % t_profilerData.memProfilerData.length();

        MemoryProfilerEvent* event = t_profilerData.memProfilerData.data() + eventIndex;
        event->frameIndex = t_profilerData.currentFrame;
        event->cycle = Platform::TimeCycles();
        event->time = Platform::TimeNow();
        event->memoryAllocated = memoryDesc.allocated;
        event->memoryFreed = memoryDesc.freed;
        event->memoryTotal = memoryDesc.total;
        event->memoryCapacity = memoryDesc.capacity;
        event->memoryLabel = memoryDesc.memoryLabel;
        event->threadId = t_profilerData.threadID;
        Platform::GetStackTrace( event->stackTrace );
    }

    void Profiler::MarkGPU( const Profiler::GPUDesc& gpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentGPUProfilerEvent++ % t_profilerData.gpuProfilerData.length();

        GPUProfilerEvent* event = t_profilerData.gpuProfilerData.data() + eventIndex;
        event->frameIndex = t_profilerData.currentFrame;
        event->gpuDuration = gpuDesc.duration;
        event->time = Platform::TimeNow();
        event->numDrawCalls = gpuDesc.numDrawCalls;
        event->numTriangles = gpuDesc.numTriangles;
        event->numCommands = gpuDesc.numCommands;
        event->threadId = t_profilerData.threadID;
    }

    void Profiler::AdvanceFrame( zp_uint64_t frameIndex )
    {
        AdvanceProfilerFrame( frameIndex );
    }

    //
    //
    //
#if 0
    ProfilerFrameEnumerator Profiler::captureFrames( const ProfilerFrameRange& range ) const
    {
        auto* profilerFrameBuffer = static_cast<zp_uint8_t*>( m_profilerFrameBuffers );

        const zp_size_t frameCount = range.endFrame - range.startFrame;

        profilerFrameBuffer += sizeof( zp_size_t ) * frameCount;

        zp_time_t minCPUTTime = ~0;
        zp_time_t maxCPUTime = 0;

        for( zp_size_t idx = 0; idx < frameCount; ++idx )
        {
            *( static_cast<zp_size_t*>( m_profilerFrameBuffers ) + idx ) = profilerFrameBuffer - static_cast<zp_uint8_t*>( m_profilerFrameBuffers );

            const zp_size_t frameCount = range.startFrame + idx;

            auto* header = reinterpret_cast<ProfilerFrameHeader*>( profilerFrameBuffer );
            header->frameCount = frameCount;
            header->cpuEvents = 0;
            header->memoryEvents = 0;
            header->gpuEvents = 0;

            profilerFrameBuffer += sizeof( ProfilerFrameHeader );

            auto* startCPUEvent = reinterpret_cast<CPUProfilerEvent*>( profilerFrameBuffer );

            for( zp_size_t i = 0; i < profilerThreadDataCount; ++i )
            {
                ProfilerThreadData* threadData = profilerThreadData[ i ];
                if( threadData != nullptr )
                {
                    auto* b = threadData->cpuProfilerData;
                    auto* e = threadData->cpuProfilerData + threadData->cpuProfilerEventCount;
                    for( ; b != e; ++b )
                    {
                        if( b->frameCount == frameCount )
                        {
                            zp_memcpy( profilerFrameBuffer, sizeof( CPUProfilerEvent ), b, sizeof( CPUProfilerEvent ) );
                            profilerFrameBuffer += sizeof( CPUProfilerEvent );
                            ++header->cpuEvents;

                            minCPUTTime = zp_min( minCPUTTime, b->startTime );
                            maxCPUTime = zp_max( maxCPUTime, b->endTime );
                        }
                    }
                }
            }

            //CPUProfilerEvent* endCPUEvent = reinterpret_cast<CPUProfilerEvent*>( profilerFrameBuffer );
            //zp_qsort3( startCPUEvent, endCPUEvent, compareCPU );
        }

        const ProfilerFrameEnumerator enumerator( range.startFrame, frameCount, m_profilerFrameBuffers );
        //enumerator.minCPUTime = minCPUTTime;
        //enumerator.maxCPUTime = maxCPUTime;

        return enumerator;
    }
#endif
}

#endif // ZP_USE_PROFILER
