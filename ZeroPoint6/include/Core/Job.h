//
// Created by phosg on 7/28/2023.
//

#ifndef ZP_JOB_H
#define ZP_JOB_H

#include "Core/Types.h"
#include "Core/Memory.h"
#include "Core/Common.h"
#include "Core/Function.h"

namespace zp
{
    struct Job;

    struct JobHandle
    {
        Job* job;
    };

    struct JobWorkArgs
    {
        JobHandle currentJob;
        JobHandle parentJob;
        zp_size_t groupId;
        zp_size_t groupIndex;
        zp_size_t index;
        Memory jobMemory;
        Memory sharedMemory;
    };

    using JobCallback = void ( * )( const JobWorkArgs& );

    using JobWorkFunc = Function<void( const JobWorkArgs& )>;

    namespace JobSystem
    {
        void Setup( MemoryLabel memoryLabel, zp_uint32_t threadCount );

        void Teardown();

        void InitializeJobThreads();

        void ExitJobThreads();

        zp_uint32_t GetThreadCount();

        zp_uint32_t GetJobQueueCount();

        void Complete( JobHandle jobHandle );

        zp_bool_t IsComplete( JobHandle jobHandle );

        //
        JobHandle Execute( JobWorkFunc func );

        JobHandle PrepareEmpty();

        JobHandle Prepare( JobWorkFunc func, JobHandle dependency );

        //
        JobHandle Dispatch( zp_size_t length, zp_size_t batchCount, JobWorkFunc func );

        JobHandle PrepareDispatch( zp_size_t length, zp_size_t batchCount, JobWorkFunc func, JobHandle dependency );

        //
        JobHandle Start( Memory jobData, JobCallback jobCallback );

        template<typename T>
        JobHandle Start( T&& jobData )
        {
            using TJob = zp_remove_reference_t<T>;
            return Start( Memory { &jobData, sizeof( TJob ) }, TJob::Execute );
        }

        //
        JobHandle Run( Memory jobData, JobCallback jobCallback );

        template<typename T>
        JobHandle Run( T&& jobData )
        {
            using TJob = zp_remove_reference_t<T>;
            return Run( Memory { &jobData, sizeof( TJob ) }, TJob::Execute );
        }

        void ScheduleBatchJobs();

        void ProcessJobs();
    }
#if 0
    static void InitializeJobThreads();

    static void ExitJobThreads();

    static void ProcessJobs();

    static PrepariedJob Start( JobWorkFunc execFunc );

    static PrepariedJob Start( JobWorkFunc execFunc, JobHandle parentJob );

    static PrepariedJob Start( JobWorkFunc execFunc, void* data, zp_size_t size );

    static PrepariedJob Start( JobWorkFunc execFunc, void* data, zp_size_t size, JobHandle parentJob );

    static void ScheduleBatchJobs();

    template<typename T>
    static PrepariedJob Start( const T& jobData )
    {
        using TJob = zp_remove_reference_t<T>;

        Wrapper <TJob> wrapper { .func = TJob::Execute, .data = jobData };
        return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ) );
    }

    template<typename T>
    static PrepariedJob Start( const T& jobData, JobHandle parentJob )
    {
        using TJob = zp_remove_reference_t<T>;

        Wrapper <TJob> wrapper { .func = TJob::Execute, .data = jobData };
        return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ), parentJob );
    }

    template<typename T>
    static PrepariedJob Start( T&& jobData )
    {
        using TJob = zp_remove_reference_t<T>;

        Wrapper <TJob> wrapper { .func = TJob::Execute, .data = zp_move( jobData ) };
        return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ) );
    }

    template<typename T>
    static PrepariedJob Start( T&& jobData, JobHandle parentJob )
    {
        using TJob = zp_remove_reference_t<T>;

        Wrapper <TJob> wrapper { .func = TJob::Execute, .data = zp_move( jobData ) };
        return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ), parentJob );
    }

    template<typename T>
    static PrepariedJob Start( void (* func)( const JobHandle& parentJobHandle, T* data ), const T& jobData )
    {
        using TJob = zp_remove_reference_t<T>;

        Wrapper <TJob> wrapper { .func = func, .data = jobData };
        return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ) );
    }

    template<typename T>
    static PrepariedJob Start( void (* func)( const JobHandle& parentJobHandle, T* data ), const T& jobData, JobHandle parentJob )
    {
        using TJob = zp_remove_reference_t<T>;

        Wrapper <TJob> wrapper { .func = func, .data = jobData };
        return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ), parentJob );
    }

    template<typename T>
    struct Wrapper
    {
        typedef void (* WrapperFunc)( const JobHandle& parentJobHandle, T* ptr );

        static void Execute( Job* job, Memory memory )
        {
            Wrapper<T>* ptr = static_cast<Wrapper<T>*>( memory.ptr );
            if( ptr && ptr->func )
            {
                ptr->func( JobHandle( job ), &ptr->data );
            }
        }

        WrapperFunc func;
        T data;
    };
};
#endif
}

#endif //ZP_JOB_H
