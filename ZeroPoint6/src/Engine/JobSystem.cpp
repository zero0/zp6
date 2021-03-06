//
// Created by phosg on 11/10/2021.
//

#include "Core/Defines.h"
#include "Core/Atomic.h"

#if ZP_USE_PROFILER

#include "Core/Profiler.h"

#endif

#include "Engine/JobSystem.h"

namespace zp
{
    enum
    {
        kJobsPerThread = 1024,
        kJobsPerThreadMask = kJobsPerThread - 1,
        kJobQueueCount = 512
    };

    namespace
    {
        thread_local Job t_jobs[kJobsPerThread];
        thread_local zp_size_t t_allocatedJobCount;

        thread_local JobQueue* t_localJobQueue;
        thread_local zp_bool_t t_isRunning;

        zp_size_t g_stealJobQueueIndex;
        zp_size_t g_totalJobQueues;
        JobQueue** g_allJobQueues;

        Job* AllocateJob()
        {
            const zp_size_t index = ++t_allocatedJobCount;
            return &t_jobs[ index & kJobsPerThreadMask ];
        }

        JobQueue* GetLocalJobQueue()
        {
            return t_localJobQueue;
        }

        JobQueue* GetStealJobQueue()
        {
            Atomic::IncrementSizeT( &g_stealJobQueueIndex );
            const zp_size_t mask = g_totalJobQueues - 1;
            return g_allJobQueues[ g_stealJobQueueIndex & mask ];
        }

        void ShutdownJobFunc( Job* job, void* data )
        {
            t_isRunning = false;
        }

        void RunJob( Job* job )
        {
            JobQueue* jobQueue = GetLocalJobQueue();
            jobQueue->push( job );
        }

        void FinishJob( Job* job )
        {
            const zp_size_t unfinishedJobs = Atomic::DecrementSizeT( &job->unfinishedJobs );
            if( unfinishedJobs == 0 )
            {
                Job* const parent = job->parent;
                Job* const next = job->next;

                //job->next = nullptr;
                //job->parent = nullptr;
                //job->func = nullptr;

                if( parent )
                {
                    FinishJob( parent );
                }

                if( next )
                {
                    RunJob( next );
                }
            }
        }

        void ExecuteJob( Job* job )
        {
            if( job->func ) job->func( job, job->padding );
            FinishJob( job );
        }

        Job* GetJob()
        {
            JobQueue* jobQueue = GetLocalJobQueue();

            Job* job = nullptr;

            if( jobQueue && !jobQueue->isEmpty())
            {
                job = jobQueue->pop();
            }
            else
            {
                JobQueue* stealJobQueue = GetStealJobQueue();
                if( stealJobQueue && stealJobQueue != jobQueue && !stealJobQueue->isEmpty())
                {
                    job = stealJobQueue->steal();
                }
                else
                {
                    zp_yield_current_thread();
                }
            }

            return job;
        }

        zp_bool_t IsJobComplete( const Job* job )
        {
            return job->unfinishedJobs == 0;
        }

        void WaitForJob( const Job* job )
        {
            while( !IsJobComplete( job ))
            {
                Job* nextJob = GetJob();
                if( nextJob )
                {
                    ExecuteJob( nextJob );
                }
            }
        }

        void InitializeJobThread( JobQueue* jobQueue )
        {
            t_localJobQueue = jobQueue;
            t_allocatedJobCount = 0;
            t_isRunning = true;
        }

        zp_uint32_t WorkerThreadExec( void* threadData )
        {
            auto* jobQueue = static_cast<JobQueue*>( threadData );

#if ZP_USE_PROFILER
            Profiler::InitializeProfilerThread();
#endif

            InitializeJobThread( jobQueue );

            while( t_isRunning )
            {
                Job* job = GetJob();
                if( job )
                {
                    ExecuteJob( job );
                }
            }

#if ZP_USE_PROFILER
            Profiler::DestroyProfilerThread();
#endif
            return 0;
        }
    }

    //
    //
    //

    PreparedJobHandle::PreparedJobHandle()
        : m_job( nullptr )
    {
    }

    PreparedJobHandle::PreparedJobHandle( Job* job )
        : m_job( job )
    {
    }

    //
    //
    //

    JobHandle::JobHandle()
        : m_job( nullptr )
    {
    }

    JobHandle::JobHandle( Job* job )
        : m_job( job )
    {
    }

    zp_bool_t JobHandle::isDone() const
    {
        return m_job == nullptr || IsJobComplete( m_job );
    }

    void JobHandle::complete()
    {
        if( m_job )
        {
            WaitForJob( m_job );
            m_job = nullptr;
        }
    }

    //
    //
    //

    JobQueue::JobQueue( MemoryLabel memoryLabel, zp_size_t count )
        : m_jobs( nullptr )
        , m_capacity( count )
        , m_bottom( 0 )
        , m_mask( count - 1 )
        , m_top( 0 )
        , memoryLabel( memoryLabel )
    {
        m_jobs = static_cast<Job**>( GetAllocator( memoryLabel )->allocate( sizeof( Job ) * m_capacity, kDefaultMemoryAlignment ));
    }

    JobQueue::~JobQueue()
    {
        GetAllocator( memoryLabel )->free( m_jobs );
        m_jobs = nullptr;
    }

    void JobQueue::push( Job* job )
    {
        const zp_size_t b = m_bottom;
        m_jobs[ b & m_mask ] = job;

        Atomic::MemoryBarrier();

        Atomic::ExchangeSizeT( &m_bottom, b + 1 );
    }

    Job* JobQueue::pop()
    {
        const zp_size_t b = m_bottom - 1;

        Atomic::ExchangeSizeT( &m_bottom, b );

        const zp_size_t t = m_top;


        if( t <= b )
        {
            Job* job = m_jobs[ b & m_mask ];

            if( t != b )
            {
                return job;
            }

            if( Atomic::CompareExchangeSizeT( &m_top, t + 1, t ) != t )
            {
                job = nullptr;
            }

            m_bottom = t + 1;
            return job;
        }
        else
        {
            m_bottom = t;
        }

        return nullptr;
    }

    Job* JobQueue::steal()
    {
        const zp_size_t t = m_top;

        Atomic::MemoryBarrier();

        const zp_size_t b = m_bottom;

        if( t < b )
        {
            Job* job = m_jobs[ t & m_mask ];

            if( Atomic::CompareExchangeSizeT( &m_top, t + 1, t ) != t )
            {
                return nullptr;
            }

            return job;
        }

        return nullptr;
    }

    zp_bool_t JobQueue::isEmpty() const
    {
        return m_bottom == m_top;
    }

    zp_size_t JobQueue::count() const
    {
        return m_bottom - m_top;
    }

    //
    //
    //

    namespace
    {
        JobSystem* g_jobSystem;
    }

    JobSystem* JobSystem::GetJobSystem()
    {
        return g_jobSystem;
    }

    JobSystem::JobSystem( MemoryLabel memoryLabel, zp_size_t threadCount )
        : m_threadHandles( nullptr )
        , m_threadCount( threadCount )
        , memoryLabel( memoryLabel )
    {
        const zp_size_t jobQueueCount = m_threadCount + 1;
        m_threadHandles = static_cast<zp_handle_t*>(GetAllocator( memoryLabel )->allocate( sizeof( zp_handle_t ) * m_threadCount, kDefaultMemoryAlignment ));
        m_jobQueues = static_cast<JobQueue**>(GetAllocator( memoryLabel )->allocate( sizeof( void* ) * jobQueueCount, kDefaultMemoryAlignment ));

        g_allJobQueues = m_jobQueues;
        g_stealJobQueueIndex = 0;
        g_totalJobQueues = jobQueueCount;
        g_jobSystem = this;

        for( zp_size_t i = 0; i < jobQueueCount; ++i )
        {
            m_jobQueues[ i ] = ZP_NEW_ARGS_( memoryLabel, JobQueue, kJobQueueCount );
        }

        char threadNameBuff[32];
        const zp_uint32_t numProcessors = GetPlatform()->GetProcessorCount();
        for( zp_size_t i = 0; i < m_threadCount; ++i )
        {
            m_threadHandles[ i ] = GetPlatform()->CreateThread( WorkerThreadExec, m_jobQueues[ i ], 1 MB, nullptr );
            GetPlatform()->SetThreadIdealProcessor( m_threadHandles[ i ], (i + 1) % numProcessors );

            zp_snprintf( threadNameBuff, "JobThread-%d", i );
            GetPlatform()->SetThreadName( m_threadHandles[ i ], threadNameBuff );
        }

        // set main thread job queue
        t_localJobQueue = m_jobQueues[ m_threadCount ];
    }

    JobSystem::~JobSystem()
    {
        const zp_size_t jobQueueCount = m_threadCount + 1;
        for( zp_size_t i = 0; i < jobQueueCount; ++i )
        {
            Job* job = AllocateJob();
            job->func = ShutdownJobFunc;
            job->parent = nullptr;
            job->next = nullptr;
            job->unfinishedJobs = 1;

            m_jobQueues[ i ]->push( job );
        }

        GetPlatform()->JoinThreads( m_threadHandles, m_threadCount );

        for( zp_size_t i = 0; i < m_threadCount; ++i )
        {
            GetPlatform()->CloseThread( m_threadHandles[ i ] );
        }

        for( zp_size_t i = 0; i < jobQueueCount; ++i )
        {
            ZP_SAFE_DELETE( JobQueue, m_jobQueues[ i ] );
        }

        ZP_ASSERT( g_jobSystem == this );
        g_jobSystem = nullptr;
        g_allJobQueues = nullptr;

        GetAllocator( memoryLabel )->free( m_threadHandles );
        GetAllocator( memoryLabel )->free( m_jobQueues );
    }

    PreparedJobHandle JobSystem::Prepare( const JobHandle& jobHandle ) const
    {
        return PreparedJobHandle( jobHandle.m_job );
    }

    PreparedJobHandle JobSystem::PrepareJob( JobWorkFunc jobWorkFunc )
    {
        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_zero_memory_array( job->padding );

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareJob( JobWorkFunc jobWorkFunc, const PreparedJobHandle& dependency )
    {
        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_zero_memory_array( job->padding );

        if( dependency.m_job ) dependency.m_job->next = job;

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size )
    {
        ZP_ASSERT( size < kJobPaddingSize );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& dependency )
    {
        ZP_ASSERT( size < kJobPaddingSize );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        if( dependency.m_job ) dependency.m_job->next = job;

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_zero_memory_array( job->padding );

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const PreparedJobHandle& dependency )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_zero_memory_array( job->padding );

        if( dependency.m_job ) dependency.m_job->next = job;

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );
        ZP_ASSERT( size < kJobPaddingSize );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        return PreparedJobHandle( job );
    }

    PreparedJobHandle JobSystem::PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& dependency )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );
        ZP_ASSERT( size < kJobPaddingSize );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        if( dependency.m_job ) dependency.m_job->next = job;

        return PreparedJobHandle( job );
    }

    JobHandle JobSystem::Schedule( const PreparedJobHandle& preparedJobHandle )
    {
        t_localJobQueue->push( preparedJobHandle.m_job );

        return JobHandle( preparedJobHandle.m_job );
    }

    JobHandle JobSystem::ScheduleJob( JobWorkFunc jobWorkFunc )
    {
        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_zero_memory_array( job->padding );

        t_localJobQueue->push( job );

        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size )
    {
        ZP_ASSERT( size < kJobPaddingSize );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        t_localJobQueue->push( job );
        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleJob( JobWorkFunc jobWorkFunc, const JobHandle& dependency )
    {
        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_zero_memory_array( job->padding );

        if( dependency.m_job ) dependency.m_job->next = job;

        t_localJobQueue->push( job );
        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const JobHandle& dependency )
    {
        ZP_ASSERT( size < kJobPaddingSize );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = nullptr;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        if( dependency.m_job ) dependency.m_job->next = job;

        t_localJobQueue->push( job );
        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        t_localJobQueue->push( job );
        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        t_localJobQueue->push( job );
        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const JobHandle& dependency )
    {
        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        if( dependency.m_job ) dependency.m_job->next = job;

        t_localJobQueue->push( job );
        return JobHandle( job );
    }

    JobHandle JobSystem::ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const JobHandle& dependency )
    {

        Atomic::IncrementSizeT( &parentJobHandle.m_job->unfinishedJobs );

        Job* job = AllocateJob();
        job->func = jobWorkFunc;
        job->parent = parentJobHandle.m_job;
        job->next = nullptr;
        job->unfinishedJobs = 1;

        zp_memcpy( job->padding, kJobPaddingSize, jobData, size );

        if( dependency.m_job ) dependency.m_job->next = job;

        t_localJobQueue->push( job );
        return JobHandle( job );
    }
}
