//
// Created by phosg on 7/28/2023.
//

#ifndef ZP_JOB_H
#define ZP_JOB_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Allocator.h"

namespace zp
{
    struct Job;

    typedef void (* JobWorkFunc)( Job* job, Memory data );

    class JobHandle
    {
    public:
        const static JobHandle Null;

        JobHandle();

        void complete();

        [[nodiscard]] zp_bool_t isComplete() const;

    private:
        explicit JobHandle( Job* job );

        Job* m_job;

        friend class JobData;

        friend class JobSystem;
    };

    class JobData
    {
    public:
        JobHandle schedule();

        JobHandle schedule( JobHandle dependency );

    private:
        explicit JobData( Job* job );

        Job* m_job;

        friend class JobSystem;
    };

    class JobSystem
    {
    ZP_NONCOPYABLE( JobSystem );

    public:
        static void Setup( MemoryLabel memoryLabel, zp_uint32_t threadCount );

        static void Teardown();

        static void InitializeJobThreads();

        static void ExitJobThreads();

        static void ProcessJobs();

        static JobData Start();

        static JobData Start( JobHandle parentJob );

        static JobData Start( JobWorkFunc execFunc );

        static JobData Start( JobWorkFunc execFunc, JobHandle parentJob );

        static JobData Start( JobWorkFunc execFunc, void* data, zp_size_t size );

        static JobData Start( JobWorkFunc execFunc, void* data, zp_size_t size, JobHandle parentJob );

        static void ScheduleBatchJobs();

        template<typename T>
        static JobData Start( const T& jobData )
        {
            using TJob = zp_remove_reference_t<T>;

            Wrapper <TJob> wrapper { .func = TJob::Execute, .data = jobData };
            return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ) );
        }

        template<typename T>
        static JobData Start( const T& jobData, JobHandle parentJob )
        {
            using TJob = zp_remove_reference_t<T>;

            Wrapper <TJob> wrapper { .func = TJob::Execute, .data = jobData };
            return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ), parentJob );
        }

        template<typename T>
        static JobData Start( T&& jobData )
        {
            using TJob = zp_remove_reference_t<T>;

            Wrapper <TJob> wrapper { .func = TJob::Execute, .data = zp_move( jobData ) };
            return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ) );
        }

        template<typename T>
        static JobData Start( T&& jobData, JobHandle parentJob )
        {
            using TJob = zp_remove_reference_t<T>;

            Wrapper <TJob> wrapper { .func = TJob::Execute, .data = zp_move( jobData ) };
            return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ), parentJob );
        }

        template<typename T>
        static JobData Start( void (* func)( const JobHandle& parentJobHandle, T* data ), const T& jobData )
        {
            using TJob = zp_remove_reference_t<T>;

            Wrapper <TJob> wrapper { .func = func, .data = jobData };
            return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ) );
        }

        template<typename T>
        static JobData Start( void (* func)( const JobHandle& parentJobHandle, T* data ), const T& jobData, JobHandle parentJob )
        {
            using TJob = zp_remove_reference_t<T>;

            Wrapper <TJob> wrapper { .func = func, .data = jobData };
            return Start( Wrapper<TJob>::Execute, &wrapper, sizeof( Wrapper < TJob > ), parentJob );
        }

    private:
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
}

#endif //ZP_JOB_H
