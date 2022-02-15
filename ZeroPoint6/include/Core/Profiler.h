//
// Created by phosg on 1/27/2022.
//

#ifndef ZP_PROFILER_H
#define ZP_PROFILER_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Allocator.h"

#if ZP_USE_PROFILER


#else // !ZP_USE_PROFILER

#endif // ZP_USE_PROFILER

#define ZP_PROFILE_CPU_BLOCK()              ProfileBlock ZP_CONCAT(__profilerBlock_,__LINE__)( __FILE__, __FUNCTION__, __LINE__, nullptr, 0 )
#define ZP_PROFILE_CPU_BLOCK_E( e )         ProfileBlock ZP_CONCAT(__profilerBlock_,__LINE__)( __FILE__, __FUNCTION__, __LINE__, #e, 0 )

#define ZP_PROFILE_CPU_BLOCK_V( v )         ProfileBlock( __FILE__, __FUNCTION__, __LINE__, nullptr, v )
#define ZP_PROFILE_CPU_BLOCK_EV( e, v )     ProfileBlock( __FILE__, __FUNCTION__, __LINE__, #e, v )

#define ZP_PROFILE_CPU_MARK( e )            zp::Profiler::MarkCPU( __FILE__, __FUNCTION__, __LINE__, #e, 0 )
#define ZP_PROFILE_CPU_MARK_V( e, v )       zp::Profiler::MarkCPU( __FILE__, __FUNCTION__, __LINE__, #e, v )

#define ZP_PROFILE_CPU_START( e )           const zp_size_t __profilerIndex_##e = zp::Profiler::StartCPU( __FILE__, __FUNCTION__, __LINE__, #e, 0 )
#define ZP_PROFILE_CPU_START_V( e, v )      const zp_size_t __profilerIndex_##e = zp::Profiler::StartCPU( __FILE__, __FUNCTION__, __LINE__, #e, v )
#define ZP_PROFILE_CPU_END( e )             zp::Profiler::EndCPU( __profilerIndex_##e )

#define ZP_PROFILE_MEM( l, a, f, t, c )     do { zp::Profiler::MemoryDesc ZP_CONCAT(__desc_,__LINE__) { a, f, t, c, l }; zp::Profiler::MarkMemory( &ZP_CONCAT(__desc_,__LINE__) ); } while( false )

#define ZP_PROFILE_GPU_START()              const zp_size_t __gpuProfilerIndex = zp::Profiler::StartGPU()
#define ZP_PROFILE_GPU_END( d, dc, t, c )   do { zp::Profiler::GPUDesc ZP_CONCAT(__desc_,__LINE__) { d, dc, t, c }; zp::Profiler::EndGPU( __gpuProfilerIndex, &ZP_CONCAT(__desc_,__LINE__) ); } while( false )

namespace zp
{
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
        explicit Profiler( MemoryLabel memoryLabel );

        ~Profiler();

    public:
        static void InitializeProfilerThread();

        static void DestroyProfilerThread();

        static void MarkCPU( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData );

        static zp_size_t StartCPU( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData );

        static void EndCPU( zp_size_t eventIndex );

        static void MarkMemory( MemoryDesc* memoryDesc );

        static zp_size_t StartGPU();

        static void EndGPU( zp_size_t eventIndex, const GPUDesc* gpuDesc );

    public:
        void advanceFrame( zp_size_t currentFrame );

    private:
        void markCPUProfilerEvent( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData );

        zp_size_t startCPUProfilerEvent( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData );

        void endCPUProfilerEvent( zp_size_t eventIndex );

        void markMemoryProfilerEvent( MemoryDesc* memoryDesc );

        zp_size_t startGPUProfilerEvent();

        void endGPUProfilerEvent( zp_size_t eventIndex, const GPUDesc* gpuDesc );

    private:
        zp_size_t m_currentFrame;

        zp_size_t m_currentCPUProfilerEvent;
        zp_size_t m_currentMemoryProfilerEvent;
        zp_size_t m_currentGPUProfilerEvent;

        zp_size_t m_maxCPUProfilerEvents;
        zp_size_t m_maxMemoryProfilerEvents;
        zp_size_t m_maxGPUProfilerEvents;

        void* m_cpuProfilerData;
        void* m_memoryProfilerData;
        void* m_gpuProfilerData;

    public:
        const MemoryLabel memoryLabel;
    };

    //
    //
    //

    class ProfileBlock
    {
    public:
        ZP_FORCEINLINE ProfileBlock( const char* filename, const char* functionName, zp_int32_t lineNumber, const char* eventName, zp_ptr_t userData )
            : m_index( Profiler::StartCPU( filename, functionName, lineNumber, eventName, userData ))
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

#endif //ZP_PROFILER_H
