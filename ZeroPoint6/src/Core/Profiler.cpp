//
// Created by phosg on 1/27/2022.
//

#include "Core/Defines.h"

#if ZP_USE_PROFILER

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Atomic.h"
#include "Core/Profiler.h"

namespace zp
{
    namespace
    {
        enum
        {
            kMaxEventStackCount = 32
        };

        Profiler* g_profiler;

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
            zp_size_t parentEvent;
            zp_ptr_t userData;
            zp_int32_t lineNumber;
            zp_uint32_t threadId;

            static zp_int32_t compare( const CPUProfilerEvent& lh, const CPUProfilerEvent& rh )
            {
                zp_int32_t cmp = zp_cmp( lh.threadId, rh.threadId );
                return cmp == 0 ? zp_cmp( lh.startTime, rh.startTime ) : cmp;
            }
        };

        struct MemoryProfilerEvent
        {
            zp_uint64_t frameIndex;
            zp_uint64_t cycle;
            zp_time_t time;
            zp_uint64_t memoryAllocated;
            zp_uint64_t memoryFreed;
            zp_uint64_t memoryTotal;
            zp_uint32_t memoryLabel;
            zp_uint32_t threadId;

            static zp_int32_t compare( const MemoryProfilerEvent& lh, const MemoryProfilerEvent& rh )
            {
                zp_int32_t cmp = zp_cmp( lh.threadId, rh.threadId );
                return cmp == 0 ? zp_cmp( lh.time, rh.time ) : cmp;
            }
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

            static zp_int32_t compare( const GPUProfilerEvent& lh, const GPUProfilerEvent& rh )
            {
                zp_int32_t cmp = zp_cmp( lh.threadId, rh.threadId );
                return cmp == 0 ? zp_cmp( lh.time, rh.time ) : cmp;
            }
        };

        struct ProfilerFrameHeader
        {
            zp_uint64_t frameIndex;
            zp_uint64_t cpuEvents;
            zp_uint64_t memoryEvents;
            zp_uint64_t gpuEvents;
        };
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
        zp_size_t eventStackCount;

        zp_size_t threadIndex;
        zp_uint32_t threadID;
    };

    namespace
    {
        thread_local ProfilerThreadData t_profilerData;
    }

    Profiler::Profiler( MemoryLabel memoryLabel, const ProfilerCreateDesc* profilerCreateDesc )
        : m_currentFrame( 0 )
        , m_maxProfilerThreads( profilerCreateDesc->maxThreadCount )
        , m_currentCPUProfilerEvent( 0 )
        , m_currentMemoryProfilerEvent( 0 )
        , m_currentGPUProfilerEvent( 0 )
        , m_maxCPUProfilerEvents( profilerCreateDesc->maxCPUEventsPerThread )
        , m_maxMemoryProfilerEvents( profilerCreateDesc->maxMemoryEventsPerThread )
        , m_maxGPUProfilerEvents( profilerCreateDesc->maxGPUEventsPerThread )
        , m_framesToCapture( profilerCreateDesc->maxFramesToCapture )
        , m_profilerFrameStride( 0 )
        , m_writeFrameIndex( 0 )
        , m_cpuProfilerData( nullptr )
        , m_memoryProfilerData( nullptr )
        , m_gpuProfilerData( nullptr )
        , m_profilerFrameBuffers( nullptr )
        , m_profilerThreadData( nullptr )
        , m_profilerDataCount( 0 )
        , m_profilerDataCapacity( profilerCreateDesc->maxThreadCount )
        , memoryLabel( memoryLabel )
    {
        g_profiler = this;

        // thread scratch buffers
        const zp_size_t cpuProfileEventSize = sizeof( CPUProfilerEvent ) * m_maxCPUProfilerEvents;
        const zp_size_t memoryProfileEventSize = sizeof( MemoryProfilerEvent ) * m_maxMemoryProfilerEvents;
        const zp_size_t gpuProfileEventSize = sizeof( GPUProfilerEvent ) * m_maxGPUProfilerEvents;

        m_cpuProfilerData = ZP_MALLOC_( memoryLabel, cpuProfileEventSize * m_maxProfilerThreads );
        m_memoryProfilerData = ZP_MALLOC_( memoryLabel, memoryProfileEventSize * m_maxProfilerThreads );
        m_gpuProfilerData = ZP_MALLOC_( memoryLabel, gpuProfileEventSize * m_maxProfilerThreads );

        zp_zero_memory( m_cpuProfilerData, cpuProfileEventSize * m_maxProfilerThreads );
        zp_zero_memory( m_memoryProfilerData, memoryProfileEventSize * m_maxProfilerThreads );
        zp_zero_memory( m_gpuProfilerData, gpuProfileEventSize * m_maxProfilerThreads );

        // thread profiler data
        m_profilerThreadData = ZP_MALLOC_T_ARRAY( memoryLabel, ProfilerThreadData*, m_profilerDataCapacity );

        // profiler frame data
        m_profilerFrameStride = ( sizeof( ProfilerFrameHeader ) + cpuProfileEventSize + memoryProfileEventSize + gpuProfileEventSize );
        const zp_size_t profilerFrameBufferSize = m_profilerFrameStride * m_framesToCapture;

        m_profilerFrameBuffers = ZP_MALLOC_( memoryLabel, profilerFrameBufferSize );

        zp_zero_memory( m_profilerFrameBuffers, profilerFrameBufferSize );
    }

    Profiler::~Profiler()
    {
        ZP_FREE_( memoryLabel, m_cpuProfilerData );
        ZP_FREE_( memoryLabel, m_memoryProfilerData );
        ZP_FREE_( memoryLabel, m_gpuProfilerData );

        ZP_FREE_( memoryLabel, m_profilerThreadData );

        ZP_FREE_( memoryLabel, m_profilerFrameBuffers );

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

    void Profiler::MarkCPU( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
    {
        const zp_size_t eventIndex = t_profilerData.currentCPUProfilerEvent++;

        CPUProfilerEvent* event = t_profilerData.cpuProfilerData + eventIndex;
        event->filename = filename;
        event->functionName = functionName;
        event->eventName = eventName;
        event->frameIndex = t_profilerData.currentFrame;
        event->startCycle = zp_time_cycle();
        event->endCycle = event->startCycle;
        event->startTime = zp_time_now();
        event->endTime = event->startTime;
        event->parentEvent = t_profilerData.eventStackCount > 0 ? t_profilerData.eventStack[ t_profilerData.eventStackCount - 1 ] : 0;
        event->userData = userData;
        event->lineNumber = lineNumber;
        event->threadId = t_profilerData.threadID;
    }

    zp_size_t Profiler::StartCPU( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
    {
        const zp_size_t eventIndex = t_profilerData.currentCPUProfilerEvent++;

        CPUProfilerEvent* event = t_profilerData.cpuProfilerData + eventIndex;
        event->filename = filename;
        event->functionName = functionName;
        event->eventName = eventName;
        event->frameIndex = t_profilerData.currentFrame;
        event->startCycle = zp_time_cycle();
        event->endCycle = event->startCycle;
        event->startTime = zp_time_now();
        event->endTime = event->startTime;
        event->parentEvent = t_profilerData.eventStackCount > 0 ? t_profilerData.eventStack[ t_profilerData.eventStackCount - 1 ] : -1;
        event->userData = userData;
        event->lineNumber = lineNumber;
        event->threadId = t_profilerData.threadID;

        ZP_ASSERT( t_profilerData.eventStackCount != kMaxEventStackCount );
        t_profilerData.eventStack[ t_profilerData.eventStackCount++ ] = eventIndex;

        return eventIndex;
    }

    void Profiler::EndCPU( zp_size_t eventIndex )
    {
        CPUProfilerEvent* event = t_profilerData.cpuProfilerData + eventIndex;

        event->endTime = zp_time_now();
        event->endCycle = zp_time_cycle();

        if( t_profilerData.eventStackCount > 0 )
        {
            --t_profilerData.eventStackCount;
        }
    }

    void Profiler::MarkMemory( const Profiler::MemoryDesc* memoryDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentMemoryProfilerEvent++;

        MemoryProfilerEvent* event = t_profilerData.memoryProfilerData + eventIndex;
        event->frameIndex = t_profilerData.currentFrame;
        event->cycle = zp_time_cycle();
        event->time = zp_time_now();
        event->memoryAllocated = memoryDesc->allocated;
        event->memoryFreed = memoryDesc->freed;
        event->memoryTotal = memoryDesc->total;
        event->memoryLabel = memoryDesc->memoryLabel;
        event->threadId = t_profilerData.threadID;
    }

    void Profiler::MarkGPU( const Profiler::GPUDesc* gpuDesc )
    {
        const zp_size_t eventIndex = t_profilerData.currentGPUProfilerEvent++;

        GPUProfilerEvent* event = t_profilerData.gpuProfilerData + eventIndex;
        event->frameIndex = t_profilerData.currentFrame;
        event->gpuDuration = gpuDesc->duration;
        event->time = zp_time_now();
        event->numDrawCalls = gpuDesc->numDrawCalls;
        event->numTriangles = gpuDesc->numTriangles;
        event->numCommands = gpuDesc->numCommands;
        event->threadId = t_profilerData.threadID;
    }

    void Profiler::AdvanceFrame( zp_uint64_t frameIndex )
    {
        g_profiler->advanceFrame( frameIndex );
    }

    //
    //
    //

    void Profiler::advanceFrame( const zp_uint64_t frameIndex )
    {
        CPUProfilerEvent advanceFrameEvent {
            __FILENAME__,
            __FUNCTION__,
            nullptr,
            m_currentFrame,
            zp_time_cycle(),
            0,
            zp_time_now(),
            0,
            0,
            0,
            __LINE__,
            zp_current_thread_id()
        };

        // copy frame data into buffer
        auto profilerFrameBuffer = static_cast<zp_uint8_t*>( m_profilerFrameBuffers );
        profilerFrameBuffer += m_writeFrameIndex * m_profilerFrameStride;

        m_writeFrameIndex = ( m_writeFrameIndex + 1 ) % m_framesToCapture;

        auto header = reinterpret_cast<ProfilerFrameHeader*>( profilerFrameBuffer );
        header->frameIndex = m_currentFrame;
        header->cpuEvents = 0;
        header->memoryEvents = 0;
        header->gpuEvents = 0;

        profilerFrameBuffer += sizeof( ProfilerFrameHeader );

        // copy cpu data
        {
            auto cpuEventHead = reinterpret_cast<CPUProfilerEvent*>( profilerFrameBuffer );

            for( zp_size_t i = 0; i < m_profilerDataCount; ++i )
            {
                ProfilerThreadData* threadData = m_profilerThreadData[ i ];
                if( threadData )
                {
                    const zp_size_t cpuEventsSize = threadData->currentCPUProfilerEvent * sizeof( CPUProfilerEvent );
                    zp_memcpy( profilerFrameBuffer, cpuEventsSize, threadData->cpuProfilerData, cpuEventsSize );

                    profilerFrameBuffer += cpuEventsSize;
                    header->cpuEvents += threadData->currentCPUProfilerEvent;
                }
            }

            auto cpuEventTail = reinterpret_cast<CPUProfilerEvent*>( profilerFrameBuffer );

            zp_qsort3( cpuEventHead, cpuEventTail, StaticFunctionComparer<CPUProfilerEvent>() );
        }

        // copy memory data
        {
            auto memoryEventHead = reinterpret_cast<MemoryProfilerEvent*>( profilerFrameBuffer );

            for( zp_size_t i = 0; i < m_profilerDataCount; ++i )
            {
                ProfilerThreadData* threadData = m_profilerThreadData[ i ];
                if( threadData )
                {
                    const zp_size_t memoryEventsSize = threadData->currentMemoryProfilerEvent * sizeof( MemoryProfilerEvent );
                    zp_memcpy( profilerFrameBuffer, memoryEventsSize, threadData->memoryProfilerData, memoryEventsSize );

                    profilerFrameBuffer += memoryEventsSize;
                    header->memoryEvents += threadData->currentMemoryProfilerEvent;
                }
            }

            auto memoryEventTail = reinterpret_cast<MemoryProfilerEvent*>( profilerFrameBuffer );

            zp_qsort3( memoryEventHead, memoryEventTail, StaticFunctionComparer<MemoryProfilerEvent>() );
        }

        // copy gpu data
        {
            auto gpuEventHead = reinterpret_cast<GPUProfilerEvent*>( profilerFrameBuffer );

            for( zp_size_t i = 0; i < m_profilerDataCount; ++i )
            {
                ProfilerThreadData* threadData = m_profilerThreadData[ i ];
                if( threadData )
                {
                    const zp_size_t gpuEventsSize = threadData->currentGPUProfilerEvent * sizeof( GPUProfilerEvent );
                    zp_memcpy( profilerFrameBuffer, gpuEventsSize, threadData->gpuProfilerData, gpuEventsSize );

                    profilerFrameBuffer += gpuEventsSize;
                    header->gpuEvents += threadData->currentGPUProfilerEvent;
                }
            }

            auto gpuEventTail = reinterpret_cast<GPUProfilerEvent*>( profilerFrameBuffer );

            zp_qsort3( gpuEventHead, gpuEventTail, StaticFunctionComparer<GPUProfilerEvent>() );
        }

        // reset for next frame
        m_currentFrame = frameIndex;
        for( zp_size_t i = 0; i < m_profilerDataCount; ++i )
        {
            ProfilerThreadData* threadData = m_profilerThreadData[ i ];
            if( threadData )
            {
                threadData->currentFrame = m_currentFrame;
                threadData->currentCPUProfilerEvent = 0;
                threadData->currentMemoryProfilerEvent = 0;
                threadData->currentGPUProfilerEvent = 0;
            }
        }
    }

    void Profiler::registerProfilerThread( ProfilerThreadData* profilerThreadData )
    {
        const zp_size_t profilerThreadIndex = Atomic::IncrementSizeT( &m_profilerDataCount ) - 1;

        profilerThreadData->cpuProfilerData = static_cast<CPUProfilerEvent*>( m_cpuProfilerData) + ( profilerThreadIndex * m_maxCPUProfilerEvents );
        profilerThreadData->memoryProfilerData = static_cast<MemoryProfilerEvent*>( m_memoryProfilerData) + ( profilerThreadIndex * m_maxMemoryProfilerEvents );
        profilerThreadData->gpuProfilerData = static_cast<GPUProfilerEvent*>( m_gpuProfilerData) + ( profilerThreadIndex * m_maxGPUProfilerEvents );

        profilerThreadData->threadIndex = profilerThreadIndex;
        profilerThreadData->threadID = zp_current_thread_id();

        m_profilerThreadData[ profilerThreadIndex ] = profilerThreadData;
    }

    void Profiler::unregisterProfilerThread( ProfilerThreadData* profilerThreadData )
    {
        profilerThreadData->currentCPUProfilerEvent = 0;
        profilerThreadData->currentGPUProfilerEvent = 0;
        profilerThreadData->currentMemoryProfilerEvent = 0;

        m_profilerThreadData[ profilerThreadData->threadIndex ] = nullptr;
    }
}

#endif // ZP_USE_PROFILER