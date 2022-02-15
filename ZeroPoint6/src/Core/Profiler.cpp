//
// Created by phosg on 1/27/2022.
//

#include "Core/Defines.h"
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

        thread_local zp_size_t t_eventStack[kMaxEventStackCount];
        thread_local zp_size_t t_eventStackCount;

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
        };

        struct GPUProfilerEvent
        {
            zp_uint64_t frameIndex;

            zp_uint64_t startCycle;
            zp_uint64_t endCycle;
            zp_time_t startTime;
            zp_time_t endTime;

            zp_time_t gpuDuration;

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

    Profiler::Profiler( MemoryLabel memoryLabel )
        : m_currentFrame( 0 )
        , m_currentCPUProfilerEvent( 0 )
        , m_currentMemoryProfilerEvent( 0 )
        , m_currentGPUProfilerEvent( 0 )
        , m_maxCPUProfilerEvents( 1024 )
        , m_maxMemoryProfilerEvents( 512 )
        , m_maxGPUProfilerEvents( 16 )
        , m_cpuProfilerData( nullptr )
        , m_memoryProfilerData( nullptr )
        , m_gpuProfilerData( nullptr )
        , memoryLabel( memoryLabel )
    {
        g_profiler = this;

        const zp_size_t cpuProfileEventSize = sizeof( CPUProfilerEvent ) * m_maxCPUProfilerEvents;
        const zp_size_t memoryProfileEventSize = sizeof( MemoryProfilerEvent ) * m_maxMemoryProfilerEvents;
        const zp_size_t gpuProfileEventSize = sizeof( GPUProfilerEvent ) * m_maxGPUProfilerEvents;

        m_cpuProfilerData = ZP_MALLOC_( memoryLabel, cpuProfileEventSize );
        m_memoryProfilerData = ZP_MALLOC_( memoryLabel, memoryProfileEventSize );
        m_gpuProfilerData = ZP_MALLOC_( memoryLabel, gpuProfileEventSize );

        zp_zero_memory( m_cpuProfilerData, cpuProfileEventSize );
        zp_zero_memory( m_memoryProfilerData, memoryProfileEventSize );
        zp_zero_memory( m_gpuProfilerData, gpuProfileEventSize );
    }

    Profiler::~Profiler()
    {
        ZP_FREE_( memoryLabel, m_cpuProfilerData );
        ZP_FREE_( memoryLabel, m_memoryProfilerData );
        ZP_FREE_( memoryLabel, m_gpuProfilerData );

        g_profiler = nullptr;
    }

    void Profiler::InitializeProfilerThread()
    {
        t_eventStackCount = 0;
        zp_zero_memory_array( t_eventStack );
    }

    void Profiler::DestroyProfilerThread()
    {
        t_eventStackCount = 0;
        zp_zero_memory_array( t_eventStack );
    }

    void Profiler::MarkCPU( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
    {
        g_profiler->markCPUProfilerEvent( filename, functionName, lineNumber, eventName, userData );
    }

    zp_size_t Profiler::StartCPU( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
    {
        return g_profiler->startCPUProfilerEvent( filename, functionName, lineNumber, eventName, userData );
    }

    void Profiler::EndCPU( zp_size_t eventIndex )
    {
        g_profiler->endCPUProfilerEvent( eventIndex );
    }

    void Profiler::MarkMemory( Profiler::MemoryDesc* memoryDesc )
    {
        g_profiler->markMemoryProfilerEvent( memoryDesc );
    }

    zp_size_t Profiler::StartGPU()
    {
        return g_profiler->startGPUProfilerEvent();
    }

    void Profiler::EndGPU( const zp_size_t eventIndex, const GPUDesc* gpuDesc )
    {
        g_profiler->endGPUProfilerEvent( eventIndex, gpuDesc );
    }

    //
    //
    //

    void Profiler::advanceFrame( const zp_size_t currentFrame )
    {
        // TODO: copy frame data into buffer

        m_currentFrame = currentFrame;
        m_currentCPUProfilerEvent = 0;
        m_currentMemoryProfilerEvent = 0;
        m_currentGPUProfilerEvent = 0;
    }

    void Profiler::markCPUProfilerEvent( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
    {
        const zp_size_t eventIndex = Atomic::IncrementSizeT( &m_currentCPUProfilerEvent ) - 1;

        auto event = reinterpret_cast<CPUProfilerEvent*>( static_cast<zp_uint8_t*>(m_cpuProfilerData ) + (sizeof( CPUProfilerEvent ) * eventIndex));
        event->filename = filename;
        event->functionName = functionName;
        event->eventName = eventName;
        event->frameIndex = m_currentFrame;
        event->startCycle = zp_time_cycle();
        event->endCycle = event->startCycle;
        event->startTime = zp_time_now();
        event->endTime = event->startTime;
        event->parentEvent = t_eventStackCount > 0 ? t_eventStack[ t_eventStackCount - 1 ] : 0;
        event->userData = userData;
        event->lineNumber = lineNumber;
        event->threadId = zp_current_thread_id();
    }

    zp_size_t Profiler::startCPUProfilerEvent( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
    {
        const zp_size_t eventIndex = Atomic::IncrementSizeT( &m_currentCPUProfilerEvent ) - 1;

        auto event = reinterpret_cast<CPUProfilerEvent*>( static_cast<zp_uint8_t*>(m_cpuProfilerData ) + (sizeof( CPUProfilerEvent ) * eventIndex));
        event->filename = filename;
        event->functionName = functionName;
        event->eventName = eventName;
        event->frameIndex = m_currentFrame;
        event->startCycle = zp_time_cycle();
        event->endCycle = event->startCycle;
        event->startTime = zp_time_now();
        event->endTime = event->startTime;
        event->parentEvent = t_eventStackCount > 0 ? t_eventStack[ t_eventStackCount - 1 ] : -1;
        event->userData = userData;
        event->lineNumber = lineNumber;
        event->threadId = zp_current_thread_id();

        ZP_ASSERT( t_eventStackCount != kMaxEventStackCount );
        t_eventStack[ t_eventStackCount++ ] = eventIndex;

        return eventIndex;
    }

    void Profiler::endCPUProfilerEvent( const zp_size_t eventIndex )
    {
        auto event = reinterpret_cast<CPUProfilerEvent*>( static_cast<zp_uint8_t*>(m_cpuProfilerData ) + (sizeof( CPUProfilerEvent ) * eventIndex));
        event->endTime = zp_time_now();
        event->endCycle = zp_time_cycle();

        if( t_eventStackCount > 0 )
        {
            --t_eventStackCount;
        }
    }

    void Profiler::markMemoryProfilerEvent( Profiler::MemoryDesc* memoryDesc )
    {
        const zp_size_t eventIndex = Atomic::IncrementSizeT( &m_currentMemoryProfilerEvent ) - 1;

        auto event = reinterpret_cast<MemoryProfilerEvent*>( static_cast<zp_uint8_t*>(m_cpuProfilerData ) + (sizeof( CPUProfilerEvent ) * eventIndex));
        event->frameIndex = m_currentFrame;
        event->cycle = zp_time_cycle();
        event->time = zp_time_now();
        event->memoryAllocated = memoryDesc->allocated;
        event->memoryFreed = memoryDesc->freed;
        event->memoryTotal = memoryDesc->total;
        event->memoryLabel = memoryDesc->memoryLabel;
        event->threadId = zp_current_thread_id();
    }

    zp_size_t Profiler::startGPUProfilerEvent()
    {
        const zp_size_t eventIndex = Atomic::IncrementSizeT( &m_currentGPUProfilerEvent ) - 1;

        auto event = reinterpret_cast<GPUProfilerEvent*>( static_cast<zp_uint8_t*>(m_cpuProfilerData ) + (sizeof( CPUProfilerEvent ) * eventIndex));
        event->frameIndex = m_currentFrame;
        event->startCycle = zp_time_cycle();
        event->endCycle = 0;
        event->startTime = zp_time_now();
        event->endTime = 0;
        event->numDrawCalls = 0;
        event->numTriangles = 0;
        event->numCommands = 0;

        zp_uint32_t threadId;
        return eventIndex;
    }

    void Profiler::endGPUProfilerEvent( const zp_size_t eventIndex, const GPUDesc* gpuDesc )
    {
        auto event = reinterpret_cast<GPUProfilerEvent*>( static_cast<zp_uint8_t*>(m_cpuProfilerData ) + (sizeof( CPUProfilerEvent ) * eventIndex));
        event->endCycle = zp_time_cycle();
        event->endTime = zp_time_now();
        event->gpuDuration = gpuDesc->duration;
        event->numDrawCalls = gpuDesc->numDrawCalls;
        event->numTriangles = gpuDesc->numTriangles;
        event->numCommands = gpuDesc->numCommands;
    }
}