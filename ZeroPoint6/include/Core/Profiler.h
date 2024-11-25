//
// Created by phosg on 1/27/2022.
//

#ifndef ZP_PROFILER_H
#define ZP_PROFILER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Allocator.h"
#include "Core/String.h"

#if ZP_USE_PROFILER
#ifndef __FILENAME__
#if ZP_OS_WINDOWS
#define __FILENAME__                        (zp_strrchr("\\" __FILE__, '\\') + 1)
#else
#define __FILENAME__                        (zp_strrchr("/" __FILE__, '/') + 1)
#endif // ZP_OS_WINDOWS
#endif // __FILENAME__

#define ZP_PROFILE_CPU_BLOCK_EV( e, v )     const zp::ProfileBlock ZP_CONCAT(__profileBlock_,__LINE__)( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = (#e), .userData = (v), .lineNumber = __LINE__, } )
#define ZP_PROFILE_CPU_BLOCK_E( e )         const zp::ProfileBlock ZP_CONCAT(__profileBlock_,__LINE__)( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = (#e), .userData = 0, .lineNumber = __LINE__, } )
#define ZP_PROFILE_CPU_BLOCK_V( v )         const zp::ProfileBlock ZP_CONCAT(__profileBlock_,__LINE__)( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = nullptr, .userData = (v), .lineNumber = __LINE__, } )
#define ZP_PROFILE_CPU_BLOCK()              const zp::ProfileBlock ZP_CONCAT(__profileBlock_,__LINE__)( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = nullptr, .userData = 0, .lineNumber = __LINE__, } )

#define ZP_PROFILE_CPU_MARK( e )            zp::Profiler::MarkCPU( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = (#e), .userData = 0, .lineNumber = __LINE__, } )
#define ZP_PROFILE_CPU_MARK_V( e, v )       zp::Profiler::MarkCPU( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = (#e), .userData = (v), .lineNumber = __LINE__, } )

#define ZP_PROFILE_CPU_DEF( e )             namespace { zp_size_t __profilerIndex_##e; }
#define ZP_PROFILE_CPU_START( e )           __profilerIndex_##e = zp::Profiler::StartCPU( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = (#e), .userData = 0, .lineNumber = __LINE__, } )
#define ZP_PROFILE_CPU_START_V( e, v )      __profilerIndex_##e = zp::Profiler::StartCPU( { .filename = __FILENAME__, .functionName = __FUNCTION__, .eventName = (#e), .userData = (v), .lineNumber = __LINE__, } )
#define ZP_PROFILE_CPU_END( e )             zp::Profiler::EndCPU( __profilerIndex_##e )

#define ZP_PROFILE_MEM( l, a, f, t, c )     zp::Profiler::MarkMemory( { .allocted = (a), .freed = (f), .total = (t), .capacity = (c), .memoryLabel = (l), } )

//#define ZP_PROFILE_GPU_START()              const zp_size_t __gpuProfilerIndex = zp::Profiler::StartGPU()
//#define ZP_PROFILE_GPU_END( d, dc, t, c )   do { zp::Profiler::GPUDesc ZP_CONCAT(__desc_,__LINE__) { d, dc, t, c }; zp::Profiler::EndGPU( __gpuProfilerIndex, &ZP_CONCAT(__desc_,__LINE__) ); } while( false )
#define ZP_PROFILE_GPU_MARK( d )            zp::Profiler::MarkGPU( { .duration = (d), .numDrawCalls = 0, .numTriangles = 0, .numCommands = 0, } )

#define ZP_PROFILE_ADVANCE_FRAME( f )       zp::Profiler::AdvanceFrame( (f) )

#else // !ZP_USE_PROFILER

#define ZP_PROFILE_CPU_BLOCK_EV( ... )      (void)0
#define ZP_PROFILE_CPU_BLOCK_E( ... )       (void)0
#define ZP_PROFILE_CPU_BLOCK_V( ... )       (void)0
#define ZP_PROFILE_CPU_BLOCK()              (void)0

#define ZP_PROFILE_CPU_MARK( ... )          (void)0
#define ZP_PROFILE_CPU_MARK_V( ... )        (void)0

#define ZP_PROFILE_CPU_DEF( ... )           (void)0
#define ZP_PROFILE_CPU_START( ... )         (void)0
#define ZP_PROFILE_CPU_START_V( ... )       (void)0
#define ZP_PROFILE_CPU_END( ... )           (void)0

#define ZP_PROFILE_MEM( ... )               (void)0

#define ZP_PROFILE_GPU_MARK( ... )          (void)0

#define ZP_PROFILE_ADVANCE_FRAME( ... )     (void)0

#endif // ZP_USE_PROFILER

#if ZP_USE_PROFILER
namespace zp
{
    struct ProfilerThreadData;

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
        zp_ptr_t memoryAddress;
        zp_uint64_t memoryAllocated;
        zp_uint64_t memoryFreed;
        zp_uint64_t memoryTotal;
        zp_uint64_t memoryCapacity;
        zp_uint32_t memoryLabel;
        zp_uint32_t threadId;
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

    struct ProfilerFrameHeader
    {
        zp_uint64_t frameIndex;
        zp_uint64_t cpuEvents;
        zp_uint64_t memoryEvents;
        zp_uint64_t gpuEvents;
    };

    struct ProfilerFrameRange
    {
        zp_uint64_t startFrame;
        zp_uint64_t endFrame;

        static ProfilerFrameRange Last( zp_uint64_t frameIndex, zp_uint64_t count )
        {
            zp_uint64_t startFrame = frameIndex < count ? 0 : frameIndex - count;
            zp_uint64_t endFrame = frameIndex;

            return { .startFrame = startFrame, .endFrame = endFrame };
        }
    };

    class ProfilerFrameEnumerator
    {
    public:
        struct Frame
        {
            const zp_size_t frameIndex;
            const CPUProfilerEvent* cpuProfilerEvents;
            const MemoryProfilerEvent* memoryProfilerEvents;
            const GPUProfilerEvent* gpuProfilerEvents;
            const zp_size_t cpuProfilerEventCount;
            const zp_size_t memoryProfilerEventCount;
            const zp_size_t gpuProfilerEventCount;
            const zp_time_t minCPUTime;
            const zp_time_t maxCPUTime;
        };

        [[nodiscard]] zp_size_t count() const
        {
            return m_count;
        }

        Frame operator[]( zp_size_t index ) const
        {
            const zp_size_t offset = *( static_cast<zp_size_t*>( m_frameBuffer ) + index );
            const zp_uint8_t* ptr = static_cast<zp_uint8_t*>( m_frameBuffer ) + offset;
            const ProfilerFrameHeader* header = reinterpret_cast<const ProfilerFrameHeader*>( ptr );

            ptr += sizeof( ProfilerFrameHeader );

            const CPUProfilerEvent* cpuProfilerEvents = nullptr;
            const GPUProfilerEvent* gpuProfilerEvents = nullptr;
            const MemoryProfilerEvent* memoryProfilerEvents = nullptr;

            if( header->cpuEvents > 0 )
            {
                cpuProfilerEvents = reinterpret_cast<const CPUProfilerEvent*>( ptr );
                ptr += sizeof( CPUProfilerEvent ) * header->cpuEvents;
            }

            if( header->memoryEvents > 0 )
            {
                memoryProfilerEvents = reinterpret_cast<const MemoryProfilerEvent*>( ptr );
                ptr += sizeof( MemoryProfilerEvent ) * header->memoryEvents;
            }

            if( header->gpuEvents > 0 )
            {
                gpuProfilerEvents = reinterpret_cast<const GPUProfilerEvent*>( ptr );
                ptr += sizeof( GPUProfilerEvent ) * header->gpuEvents;
            }

            Frame frame {
                .frameIndex = index + m_baseFrameIndex,
                .cpuProfilerEvents = cpuProfilerEvents,
                .memoryProfilerEvents = memoryProfilerEvents,
                .gpuProfilerEvents = gpuProfilerEvents,
                .cpuProfilerEventCount = header->cpuEvents,
                .memoryProfilerEventCount = header->memoryEvents,
                .gpuProfilerEventCount = header->gpuEvents,
            };
            return frame;
        }

        ProfilerFrameEnumerator() = delete;

        ~ProfilerFrameEnumerator() = default;

    private:
        ProfilerFrameEnumerator( zp_size_t baseFrameIndex, zp_size_t count, void* frameBuffer )
            : m_baseFrameIndex( baseFrameIndex )
            , m_count( count )
            , m_frameBuffer( frameBuffer )
        {
        }


        zp_size_t m_baseFrameIndex;
        zp_size_t m_count;

        void* m_frameBuffer;

        friend class Profiler;
    };

    struct ProfilerCreateDesc
    {
        zp_size_t maxThreadCount;
        zp_size_t maxCPUEventsPerThread;
        zp_size_t maxMemoryEventsPerThread;
        zp_size_t maxGPUEventsPerThread;
        zp_size_t maxFramesToCapture;
    };

    class Profiler
    {
    ZP_NONCOPYABLE( Profiler );

    public:
        struct CPUDesc
        {
            const char* filename;
            const char* functionName;
            const char* eventName;
            zp_ptr_t userData;
            zp_int32_t lineNumber;
        };

        struct MemoryDesc
        {
            zp_uint64_t allocated;
            zp_uint64_t freed;
            zp_uint64_t total;
            zp_uint64_t capacity;
            MemoryLabel memoryLabel;
        };

        struct GPUDesc
        {
            zp_time_t duration;
            zp_uint64_t numDrawCalls;
            zp_uint64_t numTriangles;
            zp_uint64_t numCommands;
        };

    public:
        Profiler( MemoryLabel memoryLabel, const ProfilerCreateDesc& profilerCreateDesc );

        ~Profiler();

    public:
        static void InitializeProfilerThread();

        static void DestroyProfilerThread();

        static void MarkCPU( const CPUDesc& cpuDesc );

        static zp_size_t StartCPU( const CPUDesc& cpuDesc );

        static void EndCPU( zp_size_t eventIndex );

        static void MarkMemory( const MemoryDesc& memoryDesc );

        static void MarkGPU( const GPUDesc& gpuDesc );

        static void AdvanceFrame( zp_uint64_t frameIndex );

    public:
        ProfilerFrameEnumerator captureFrames( const ProfilerFrameRange& range ) const;

    private:
        void advanceFrame( zp_uint64_t frameIndex );

        void registerProfilerThread( ProfilerThreadData* profilerThreadData );

        void unregisterProfilerThread( ProfilerThreadData* profilerThreadData );

    private:
        zp_uint64_t m_currentFrame;

        zp_size_t m_maxProfilerThreads;

        zp_size_t m_currentCPUProfilerEvent;
        zp_size_t m_currentMemoryProfilerEvent;
        zp_size_t m_currentGPUProfilerEvent;

        zp_size_t m_maxCPUProfilerEvents;
        zp_size_t m_maxMemoryProfilerEvents;
        zp_size_t m_maxGPUProfilerEvents;

        zp_size_t m_framesToCapture;
        zp_size_t m_profilerFrameStride;

        CPUProfilerEvent* m_cpuProfilerData;
        MemoryProfilerEvent* m_memoryProfilerData;
        GPUProfilerEvent* m_gpuProfilerData;

        void* m_profilerFrameBuffers;

        ProfilerThreadData** m_profilerThreadData;
        zp_size_t m_profilerDataThreadCount;
        zp_size_t m_profilerDataThreadCapacity;

    public:
        const MemoryLabel memoryLabel;
    };

    Profiler* GetProfiler();

    //
    //
    //

    class ProfileBlock
    {
    ZP_NONCOPYABLE( ProfileBlock );

    public:
        ZP_FORCEINLINE explicit ProfileBlock( const Profiler::CPUDesc& cpuDesc )
            : m_index( Profiler::StartCPU( cpuDesc ) )
        {
        }

        ZP_FORCEINLINE ~ProfileBlock()
        {
            Profiler::EndCPU( m_index );
        }

    private:
        const zp_size_t m_index;
    };
}
#endif // ZP_USE_PROFILER

#endif //ZP_PROFILER_H
