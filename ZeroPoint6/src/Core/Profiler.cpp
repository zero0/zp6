//
// Created by phosg on 1/27/2022.
//

#include "Core/Defines.h"

#if ZP_USE_PROFILER

#include "Core/Math.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Atomic.h"
#include "Core/Profiler.h"
#include "Platform/Platform.h"

namespace zp
{
    namespace
    {
        enum
        {
            kMaxEventStackCount = 32
        };

        Profiler* g_profiler;

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

    struct ProfilerThreadData
    {
        zp_uint64_t currentFrame;

        CPUProfilerEvent* cpuProfilerData;
        MemoryProfilerEvent* memoryProfilerData;
        GPUProfilerEvent* gpuProfilerData;
        zp_size_t eventStack[kMaxEventStackCount];

        zp_size_t currentCPUProfilerEvent;
        zp_size_t currentMemoryProfilerEvent;
        zp_size_t currentGPUProfilerEvent;

        zp_size_t cpuProfilerEventCount;
        zp_size_t memoryProfilerEventCount;
        zp_size_t gpuProfilerEventCount;

        zp_size_t eventStackCount;

        zp_size_t threadIndex;
        zp_uint32_t threadID;
    };

    namespace
    {
        thread_local ProfilerThreadData t_profilerData;
    }

    Profiler::Profiler( MemoryLabel memoryLabel, const ProfilerCreateDesc& profilerCreateDesc )
        : m_currentFrame( 0 )
        , m_maxProfilerThreads( profilerCreateDesc.maxThreadCount )
        , m_currentCPUProfilerEvent( 0 )
        , m_currentMemoryProfilerEvent( 0 )
        , m_currentGPUProfilerEvent( 0 )
        , m_maxCPUProfilerEvents( profilerCreateDesc.maxCPUEventsPerThread )
        , m_maxMemoryProfilerEvents( profilerCreateDesc.maxMemoryEventsPerThread )
        , m_maxGPUProfilerEvents( profilerCreateDesc.maxGPUEventsPerThread )
        , m_framesToCapture( profilerCreateDesc.maxFramesToCapture )
        , m_profilerFrameStride( 0 )
        , m_cpuProfilerData( nullptr )
        , m_memoryProfilerData( nullptr )
        , m_gpuProfilerData( nullptr )
        , m_profilerFrameBuffers( nullptr )
        , m_profilerThreadData( nullptr )
        , m_profilerDataThreadCount( 0 )
        , m_profilerDataThreadCapacity( profilerCreateDesc.maxThreadCount )
        , memoryLabel( memoryLabel )
    {
        g_profiler = this;

        // thread scratch buffers
        const zp_size_t cpuProfileEventSize = sizeof( CPUProfilerEvent ) * m_maxCPUProfilerEvents;
        const zp_size_t memoryProfileEventSize = sizeof( MemoryProfilerEvent ) * m_maxMemoryProfilerEvents;
        const zp_size_t gpuProfileEventSize = sizeof( GPUProfilerEvent ) * m_maxGPUProfilerEvents;

        const zp_size_t cpuProfilerEventAllocationSize = cpuProfileEventSize * m_maxProfilerThreads;
        const zp_size_t memoryProfilerEventAllocationSize = memoryProfileEventSize * m_maxProfilerThreads;
        const zp_size_t gpuProfilerEventAllocationSize = gpuProfileEventSize * m_maxProfilerThreads;

        m_cpuProfilerData = ZP_MALLOC_T_ARRAY( memoryLabel, CPUProfilerEvent, cpuProfilerEventAllocationSize );
        m_memoryProfilerData = ZP_MALLOC_T_ARRAY( memoryLabel, MemoryProfilerEvent, memoryProfilerEventAllocationSize );
        m_gpuProfilerData = ZP_MALLOC_T_ARRAY( memoryLabel, GPUProfilerEvent, gpuProfilerEventAllocationSize );

        zp_zero_memory( m_cpuProfilerData, cpuProfilerEventAllocationSize );
        zp_zero_memory( m_memoryProfilerData, memoryProfilerEventAllocationSize );
        zp_zero_memory( m_gpuProfilerData, gpuProfilerEventAllocationSize );

        // thread profiler data
        m_profilerThreadData = ZP_MALLOC_T_ARRAY( memoryLabel, ProfilerThreadData*, m_profilerDataThreadCapacity );

        // profiler frame data
        m_profilerFrameStride = ( sizeof( ProfilerFrameHeader ) + cpuProfileEventSize + memoryProfileEventSize + gpuProfileEventSize );
        const zp_size_t profilerFrameBufferAllocationSize = m_profilerFrameStride * m_framesToCapture;

        m_profilerFrameBuffers = ZP_MALLOC( memoryLabel, profilerFrameBufferAllocationSize );

        zp_zero_memory( m_profilerFrameBuffers, profilerFrameBufferAllocationSize );
    }

    Profiler::~Profiler()
    {
        ZP_FREE( memoryLabel, m_cpuProfilerData );
        ZP_FREE( memoryLabel, m_memoryProfilerData );
        ZP_FREE( memoryLabel, m_gpuProfilerData );

        ZP_FREE( memoryLabel, m_profilerThreadData );

        ZP_FREE( memoryLabel, m_profilerFrameBuffers );

        g_profiler = nullptr;
    }

    void Profiler::InitializeProfilerThread()
    {
        zp_zero_memory( &t_profilerData );

        g_profiler->registerProfilerThread( &t_profilerData );
    }

    void Profiler::DestroyProfilerThread()
    {
        g_profiler->unregisterProfilerThread( &t_profilerData );

        zp_zero_memory( &t_profilerData );
    }

    void Profiler::MarkCPU( const CPUDesc& cpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentCPUProfilerEvent++ % t_profilerData.cpuProfilerEventCount;

        CPUProfilerEvent* event = t_profilerData.cpuProfilerData + eventIndex;
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
    }

    zp_size_t Profiler::StartCPU( const CPUDesc& cpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentCPUProfilerEvent++ % t_profilerData.cpuProfilerEventCount;

        CPUProfilerEvent* event = t_profilerData.cpuProfilerData + eventIndex;
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

        ZP_ASSERT( t_profilerData.eventStackCount != kMaxEventStackCount );
        t_profilerData.eventStack[ t_profilerData.eventStackCount++ ] = eventIndex;

        return eventIndex;
    }

    void Profiler::EndCPU( zp_size_t eventIndex )
    {
        CPUProfilerEvent* event = t_profilerData.cpuProfilerData + eventIndex;

        event->endTime = Platform::TimeNow();
        event->endCycle = Platform::TimeCycles();

        ZP_ASSERT(t_profilerData.eventStackCount > 0 );
        //if( t_profilerData.eventStackCount > 0 )
        {
            --t_profilerData.eventStackCount;
        }
    }

    void Profiler::MarkMemory( const Profiler::MemoryDesc& memoryDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentMemoryProfilerEvent++ % t_profilerData.memoryProfilerEventCount;

        MemoryProfilerEvent* event = t_profilerData.memoryProfilerData + eventIndex;
        event->frameIndex = t_profilerData.currentFrame;
        event->cycle = Platform::TimeCycles();
        event->time = Platform::TimeNow();
        event->memoryAllocated = memoryDesc.allocated;
        event->memoryFreed = memoryDesc.freed;
        event->memoryTotal = memoryDesc.total;
        event->memoryCapacity = memoryDesc.capacity;
        event->memoryLabel = memoryDesc.memoryLabel;
        event->threadId = t_profilerData.threadID;
    }

    void Profiler::MarkGPU( const Profiler::GPUDesc& gpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentGPUProfilerEvent++ % t_profilerData.gpuProfilerEventCount;

        GPUProfilerEvent* event = t_profilerData.gpuProfilerData + eventIndex;
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
        g_profiler->advanceFrame( frameIndex );
    }

    //
    //
    //

    ProfilerFrameEnumerator Profiler::captureFrames( const ProfilerFrameRange& range ) const
    {
        auto profilerFrameBuffer = static_cast<zp_uint8_t*>( m_profilerFrameBuffers );

        const zp_size_t frameCount = range.endFrame - range.startFrame;

        profilerFrameBuffer += sizeof( zp_size_t ) * frameCount;

        zp_time_t minCPUTTime = ~0;
        zp_time_t maxCPUTime = 0;

        for( zp_size_t idx = 0; idx < frameCount; ++idx )
        {
            *( static_cast<zp_size_t*>( m_profilerFrameBuffers ) + idx ) = profilerFrameBuffer - static_cast<zp_uint8_t*>( m_profilerFrameBuffers );

            const zp_size_t frameIndex = range.startFrame + idx;

            auto header = reinterpret_cast<ProfilerFrameHeader*>( profilerFrameBuffer );
            header->frameIndex = frameIndex;
            header->cpuEvents = 0;
            header->memoryEvents = 0;
            header->gpuEvents = 0;

            profilerFrameBuffer += sizeof( ProfilerFrameHeader );

            CPUProfilerEvent* startCPUEvent = reinterpret_cast<CPUProfilerEvent*>( profilerFrameBuffer );

            for( zp_size_t i = 0; i < m_profilerDataThreadCount; ++i )
            {
                ProfilerThreadData* threadData = m_profilerThreadData[ i ];
                if( threadData )
                {
                    auto b = threadData->cpuProfilerData;
                    auto e = threadData->cpuProfilerData + threadData->cpuProfilerEventCount;
                    for( ; b != e; ++b )
                    {
                        if( b->frameIndex == frameIndex )
                        {
                            zp_memcpy( profilerFrameBuffer, sizeof( CPUProfilerEvent ), b, sizeof( CPUProfilerEvent ) );
                            profilerFrameBuffer += sizeof( CPUProfilerEvent );
                            ++header->cpuEvents;

                            minCPUTTime = zp_min( minCPUTTime, b->startTime);
                            maxCPUTime = zp_max( maxCPUTime, b->endTime);
                        }
                    }
                }
            }

            //CPUProfilerEvent* endCPUEvent = reinterpret_cast<CPUProfilerEvent*>( profilerFrameBuffer );
            //zp_qsort3( startCPUEvent, endCPUEvent, compareCPU );
        }

        ProfilerFrameEnumerator enumerator( range.startFrame, frameCount, m_profilerFrameBuffers );
        //enumerator.minCPUTime = minCPUTTime;
        //enumerator.maxCPUTime = maxCPUTime;

        return enumerator;
    }

    void Profiler::advanceFrame( const zp_uint64_t frameIndex )
    {
        m_currentFrame = frameIndex;
        for( zp_size_t i = 0; i < m_profilerDataThreadCount; ++i )
        {
            ProfilerThreadData* threadData = m_profilerThreadData[ i ];
            if( threadData )
            {
                Atomic::Exchange( &threadData->currentFrame, frameIndex );
            }
        }
    }

    void Profiler::registerProfilerThread( ProfilerThreadData* profilerThreadData )
    {
        const zp_size_t profilerThreadIndex = Atomic::IncrementSizeT( &m_profilerDataThreadCount ) - 1;

        profilerThreadData->cpuProfilerData = m_cpuProfilerData + ( profilerThreadIndex * m_maxCPUProfilerEvents );
        profilerThreadData->memoryProfilerData = m_memoryProfilerData + ( profilerThreadIndex * m_maxMemoryProfilerEvents );
        profilerThreadData->gpuProfilerData = m_gpuProfilerData + ( profilerThreadIndex * m_maxGPUProfilerEvents );

        profilerThreadData->cpuProfilerEventCount = m_maxCPUProfilerEvents;
        profilerThreadData->memoryProfilerEventCount = m_maxMemoryProfilerEvents;
        profilerThreadData->gpuProfilerEventCount = m_maxGPUProfilerEvents;

        profilerThreadData->threadIndex = profilerThreadIndex;
        profilerThreadData->threadID = Platform::GetCurrentThreadId();

        m_profilerThreadData[ profilerThreadIndex ] = profilerThreadData;
    }

    void Profiler::unregisterProfilerThread( ProfilerThreadData* profilerThreadData )
    {
        m_profilerThreadData[ profilerThreadData->threadIndex ] = nullptr;
    }

    Profiler* GetProfiler()
    {
        return g_profiler;
    }
}

#endif // ZP_USE_PROFILER
