//
// Created by phosg on 7/28/2023.
//

#include "Core/Job.h"
#include "Core/Vector.h"
#include "Core/Atomic.h"
#include "Platform/Platform.h"

using namespace zp;


namespace
{
    enum
    {
        kJobPayload = 4 KB
    };

    struct JobHeader
    {
        Job* parent;
        JobWorkFunc execFunc;
        zp_int32_t uncompletedJobs;
        zp_int32_t numDependencies;
        zp_size_t jobDataSize;
    };
}

namespace zp
{

    struct Job
    {
        JobHeader* header()
        {
            JobHeader* h = reinterpret_cast<JobHeader*>( m_jobData );
            return h;
        }

        Memory data()
        {
            JobHeader* h = header();
            return {
                .ptr = m_jobData + sizeof( JobHeader ),
                .size = h->jobDataSize
            };
        }

        void execute()
        {
            JobWorkFunc exec = header()->execFunc;
            if( exec )
            {
                exec( this, data() );
            }
        }

        void addDependency( Job* job )
        {
            JobHeader* h = header();
            Job** dep = reinterpret_cast<Job**>( m_jobData + ( kJobPayload - sizeof( Job* ) * ( 1 + h->numDependencies ) ) );
            dep[ h->numDependencies ] = job;
            ++h->numDependencies;
        }

        MemoryArray<Job*> dependencies()
        {
            JobHeader* h = header();
            return {
                .ptr = h->numDependencies == 0 ? nullptr : reinterpret_cast<Job**>( m_jobData + ( kJobPayload - sizeof( Job* ) * ( 1 + h->numDependencies ) ) ),
                .length = static_cast<zp_size_t >(h->numDependencies)
            };
        }

        zp_uint8_t m_jobData[kJobPayload];
    };
}

namespace
{
    enum
    {
        kJobsPerThread = 512,
        kJobsPerThreadMask = kJobsPerThread - 1,

        kJobQueueCount = 256,
        kJobQueueCountMask = kJobQueueCount - 1,
    };

    ZP_STATIC_ASSERT( zp_is_pow2( kJobQueueCount ) );
    ZP_STATIC_ASSERT( zp_is_pow2( kJobsPerThread ) );
}

namespace
{
    class JobQueue
    {
    ZP_NONCOPYABLE( JobQueue );

    public:
        [[nodiscard]] zp_bool_t empty() const;

        void pushBack( Job* job );

        Job* popBack();

        Job* stealPopFront();

    private:
        FixedArray<Job*, kJobQueueCount> m_jobs;
        zp_int32_t m_back;
        zp_int32_t m_front;
    };

    zp_bool_t JobQueue::empty() const
    {
        return m_back == m_front;
    }

    void JobQueue::pushBack( Job* job )
    {
        const zp_int32_t b = m_back;
        m_jobs[ b & kJobQueueCountMask ] = job;
        Atomic::Exchange( &m_back, b + 1 );
    }

    Job* JobQueue::popBack()
    {
        const zp_int32_t b = m_back - 1;
        Atomic::Exchange( &m_back, b );

        const zp_int32_t t = m_front;

        Job* job = nullptr;

        if( t <= b )
        {
            job = m_jobs[ b & kJobQueueCountMask ];
            if( t != b )
            {
                return job;
            }

            if( Atomic::CompareExchange( &m_front, t + 1, t ) != t )
            {
                job = nullptr;
            }

            m_back = t + 1;
        }
        else
        {
            m_back = t;
        }

        return job;
    }

    Job* JobQueue::stealPopFront()
    {
        const zp_int32_t t = m_front;

        Atomic::MemoryBarrier();

        const zp_int32_t b = m_back;

        Job* job = nullptr;

        if( t < b )
        {
            job = m_jobs[ t & kJobQueueCountMask ];

            if( Atomic::CompareExchange( &m_front, t + 1, t ) != t )
            {
                job = nullptr;
            }
        }

        return job;
    }
}

namespace
{
    thread_local Job t_jobs[kJobsPerThread];

    thread_local zp_size_t t_allocatedJobCount;

    thread_local JobQueue* t_localJobQueue;

    thread_local JobQueue* t_localBatchJobQueue;

    thread_local zp_bool_t t_isRunning;

    MemoryArray<JobQueue> g_allQueues;

    MemoryArray<JobQueue*> g_allJobQueues;

    MemoryArray<JobQueue*> g_allBatchJobQueues;

    zp_size_t g_stealJobQueueIndex;

    struct JobThreadInfo
    {
        JobQueue* localJobQueue;
        JobQueue* localBatchQueue;
        zp_handle_t threadHandle;
        zp_uint32_t threadID;
    };
    MemoryArray<JobThreadInfo> g_jobThreadHandles;

    MemoryLabel g_memoryLabel;

    //
    //
    //

    JobQueue* GetLocalJobQueue()
    {
        return t_localJobQueue;
    }

    JobQueue* GetLocalBatchJobQueue()
    {
        return t_localBatchJobQueue;
    }

    JobQueue* GetStealJobQueue()
    {
        const zp_size_t index = Atomic::IncrementSizeT( &g_stealJobQueueIndex ) - 1;
        return g_allJobQueues[ index % g_allJobQueues.length ];
    }

    Job* AllocateJob()
    {
        const zp_size_t index = ++t_allocatedJobCount;
        return &t_jobs[ index & kJobsPerThreadMask ];
    }

    void EnqueueLocalJob( Job* job )
    {
        JobQueue* jobQueue = GetLocalJobQueue();
        jobQueue->pushBack( job );
    }

    void FinishJob( Job* job )
    {
        JobHeader* header = job->header();

        const zp_int32_t unfinishedJobs = Atomic::Decrement( &header->uncompletedJobs );

        if( unfinishedJobs == 0 )
        {
            Job* parent = header->parent;

            if( parent )
            {
                FinishJob( parent );
            }

            MemoryArray<Job*> deps = job->dependencies();
            for( Job* dep : deps )
            {
                EnqueueLocalJob( dep );
            }

            Atomic::Decrement( &header->uncompletedJobs );

            // clear job memory
            Memory mem = job->data();
            zp_zero_memory( mem.ptr, mem.size );
        }
    }

    void ExecuteJob( Job* job )
    {
        job->execute();

        FinishJob( job );
    }

    Job* GetJob()
    {
        Job* job = nullptr;

        JobQueue* jobQueue = GetLocalJobQueue();
        if( jobQueue && !jobQueue->empty() )
        {
            job = jobQueue->popBack();
        }
        else
        {
            JobQueue* stealJobQueue = GetStealJobQueue();
            if( stealJobQueue != jobQueue && !stealJobQueue->empty() )
            {
                job = stealJobQueue->stealPopFront();
            }
        }

        if( job == nullptr )
        {
            zp_yield_current_thread();
        }

        return job;
    }

    void FlushBatchJobs()
    {
        JobQueue* batchQueue = GetLocalBatchJobQueue();

        zp_size_t index = 0;
        while( !batchQueue->empty() )
        {
            Job* job = batchQueue->popBack();

            g_allJobQueues[ index % g_allJobQueues.length ]->pushBack( job );
            ++index;
        }
    }

    void FlushBatchJobsLocally()
    {
        JobQueue* jobQueue = GetLocalJobQueue();
        JobQueue* batchQueue = GetLocalBatchJobQueue();

        while( !batchQueue->empty() )
        {
            Job* job = batchQueue->popBack();
            jobQueue->pushBack( job );
        }
    }

    zp_bool_t IsJobComplete( Job* job )
    {
        return job->header()->uncompletedJobs <= 0;
    }

    void WaitForJobComplete( Job* job )
    {
        while( !IsJobComplete( job ) )
        {
            Job* waitJob = GetJob();
            if( waitJob )
            {
                ExecuteJob( waitJob );
            }
        }
    }

    //
    //
    //

    void ShutdownThreadJob( Job* job, Memory data )
    {
        t_isRunning = false;
    }

    zp_uint32_t WorkerThreadFunc( void* threadData )
    {
        zp_ptr_t index = reinterpret_cast<zp_ptr_t>( threadData);
        JobThreadInfo& info = g_jobThreadHandles[ index ];

        zp_zero_memory_array( t_jobs );
        t_isRunning = true;
        t_allocatedJobCount = 0;
        t_localJobQueue = info.localJobQueue;
        t_localBatchJobQueue = info.localBatchQueue;

        while( t_isRunning )
        {
            Job* job = GetJob();
            if( job )
            {
                ExecuteJob( job );
            }

            zp_yield_current_thread();
        }

        t_allocatedJobCount = 0;
        t_localBatchJobQueue = nullptr;
        t_localJobQueue = nullptr;

        return 0;
    }
}

//
//
//

const JobHandle JobHandle::Null = JobHandle( nullptr );

JobHandle::JobHandle()
    : m_job( nullptr )
{
}

JobHandle::JobHandle( Job* job )
    : m_job( job )
{
}

void JobHandle::complete()
{
    if( m_job )
    {
        WaitForJobComplete( m_job );
        m_job = nullptr;
    }
}

zp_bool_t JobHandle::isComplete() const
{
    return m_job == nullptr || IsJobComplete( m_job );
}

//
//
//

JobData::JobData( Job* job )
    : m_job( job )
{
}

JobHandle JobData::schedule()
{
    Job* job = m_job;
    m_job = nullptr;

    if( job )
    {
        GetLocalBatchJobQueue()->pushBack( job );
    }

    return JobHandle( job );
}

JobHandle JobData::schedule( JobHandle dependency )
{
    Job* job = m_job;
    m_job = nullptr;

    if( job )
    {
        if( dependency.m_job && !dependency.isComplete() )
        {
            dependency.m_job->addDependency( job );
        }
        else
        {
            GetLocalBatchJobQueue()->pushBack( job );
        }
    }

    return JobHandle( job );
}

//
//
//

void JobSystem::Setup( MemoryLabel memoryLabel, zp_uint32_t threadCount )
{
    if( threadCount == 0 )
    {
        threadCount = Platform::GetProcessorCount() - 1;
    }
    else
    {
        threadCount = zp_min( threadCount, Platform::GetProcessorCount() - 1 );
    }

    g_stealJobQueueIndex = 0;
    g_memoryLabel = memoryLabel;

    const zp_uint32_t jobCount = threadCount + 1;

    g_allQueues = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobQueue, jobCount * 2 ),
        .length = jobCount * 2
    };
    zp_zero_memory_array( g_allQueues.ptr, g_allQueues.length );

    g_allJobQueues = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobQueue*, jobCount ),
        .length = jobCount
    };

    g_allBatchJobQueues = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobQueue*, jobCount ),
        .length = jobCount
    };

    for( zp_uint32_t i = 0; i < jobCount; ++i )
    {
        g_allJobQueues[ i ] = &g_allQueues[ i ];
        g_allBatchJobQueues[ i ] = &g_allQueues[ i + jobCount ];
    }

    g_jobThreadHandles = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobThreadInfo, threadCount ),
        .length = threadCount
    };
    zp_zero_memory_array( g_jobThreadHandles.ptr, g_jobThreadHandles.length );
}

void JobSystem::Teardown()
{
    ZP_FREE_( g_memoryLabel, g_jobThreadHandles.ptr );

    ZP_FREE_( g_memoryLabel, g_allBatchJobQueues.ptr );
    ZP_FREE_( g_memoryLabel, g_allJobQueues.ptr );
    ZP_FREE_( g_memoryLabel, g_allQueues.ptr );
}

void JobSystem::InitializeJobThreads()
{
    const zp_size_t stackSize = 1 MB;
    MutableFixedString<64> threadName;

    const zp_uint32_t numAvailableProcessors = Platform::GetProcessorCount() - 1;
    for( zp_uint32_t i = 0; i < g_jobThreadHandles.length; ++i )
    {
        JobThreadInfo& info = g_jobThreadHandles[ i ];
        info.localJobQueue = g_allJobQueues[ i ];
        info.localBatchQueue = g_allBatchJobQueues[ i ];

        info.threadHandle = Platform::CreateThread( WorkerThreadFunc, reinterpret_cast<void*>( static_cast<zp_ptr_t>( i ) ), stackSize, &info.threadID );

        Platform::SetThreadIdealProcessor( info.threadHandle, 1 + ( i % numAvailableProcessors ) ); // proc 0 is used for main thread

        threadName.format( "JobThread-%d %d", info.threadID, i );

        Platform::SetThreadName( info.threadHandle, threadName );
    }

    // setup main thread job info
    zp_zero_memory_array( t_jobs );
    t_isRunning = true;
    t_allocatedJobCount = 0;
    t_localJobQueue = g_allJobQueues[ g_jobThreadHandles.length ];
    t_localBatchJobQueue = g_allBatchJobQueues[ g_jobThreadHandles.length ];
}

void JobSystem::ExitJobThreads()
{
    // queue shutdown job
    for( zp_size_t i = 0; i < g_allJobQueues.length; ++i )
    {
        Job* job = AllocateJob();

        JobHeader* header = job->header();
        *header = {
            .parent = nullptr,
            .execFunc = ShutdownThreadJob,
            .uncompletedJobs = 1,
            .numDependencies = 0,
            .jobDataSize = 0
        };

        g_allJobQueues[ i ]->pushBack( job );
    }

    // wait for all threads to finish
    zp_handle_t threadHandles[g_jobThreadHandles.length];
    for( zp_size_t i = 0; i < g_jobThreadHandles.length; ++i )
    {
        threadHandles[ i ] = g_jobThreadHandles[ i ].threadHandle;
    }

    Platform::JoinThreads( threadHandles, g_jobThreadHandles.length );

    // close threads
    for( zp_size_t i = 0; i < g_jobThreadHandles.length; ++i )
    {
        Platform::CloseThread( threadHandles[ i ] );
        g_jobThreadHandles[ i ] = {};
    }
}

JobData JobSystem::Start()
{
    Job* job = AllocateJob();

    JobHeader* header = job->header();
    *header = {
        .parent = nullptr,
        .execFunc = nullptr,
        .uncompletedJobs = 1,
        .numDependencies = 0,
        .jobDataSize = 0
    };

    return JobData( job );
}

JobData JobSystem::Start( JobHandle parentJob )
{
    if( parentJob.m_job )
    {
        Atomic::Increment( &parentJob.m_job->header()->uncompletedJobs );
    }

    Job* job = AllocateJob();

    JobHeader* header = job->header();
    *header = {
        .parent = parentJob.m_job,
        .execFunc = nullptr,
        .uncompletedJobs = 1,
        .numDependencies = 0,
        .jobDataSize = 0
    };

    return JobData( job );
}

JobData JobSystem::Start( JobWorkFunc execFunc )
{
    Job* job = AllocateJob();

    JobHeader* header = job->header();
    *header = {
        .parent = nullptr,
        .execFunc = execFunc,
        .uncompletedJobs = 1,
        .numDependencies = 0,
        .jobDataSize = 0
    };

    return JobData( job );
}

JobData JobSystem::Start( JobWorkFunc execFunc, JobHandle parentJob )
{
    if( parentJob.m_job )
    {
        Atomic::Increment( &parentJob.m_job->header()->uncompletedJobs );
    }

    Job* job = AllocateJob();

    JobHeader* header = job->header();
    *header = {
        .parent = parentJob.m_job,
        .execFunc = execFunc,
        .uncompletedJobs = 1,
        .numDependencies = 0,
        .jobDataSize = 0
    };

    return JobData( job );
}

JobData JobSystem::Start( JobWorkFunc execFunc, void* data, zp_size_t size )
{
    Job* job = AllocateJob();

    JobHeader* header = job->header();
    *header = {
        .parent = nullptr,
        .execFunc = execFunc,
        .uncompletedJobs = 1,
        .numDependencies = 0,
        .jobDataSize = size
    };

    if( data && size > 0 )
    {
        Memory mem = job->data();
        zp_memcpy( mem.ptr, mem.size, data, size );
    }

    return JobData( job );
}

JobData JobSystem::Start( JobWorkFunc execFunc, void* data, zp_size_t size, JobHandle parentJob )
{
    if( parentJob.m_job )
    {
        Atomic::Increment( &parentJob.m_job->header()->uncompletedJobs );
    }

    Job* job = AllocateJob();

    JobHeader* header = job->header();
    *header = {
        .parent = parentJob.m_job,
        .execFunc = execFunc,
        .uncompletedJobs = 1,
        .numDependencies = 0,
        .jobDataSize = size
    };

    if( data && size > 0 )
    {
        Memory mem = job->data();
        zp_memcpy( mem.ptr, mem.size, data, size );
    }

    return JobData( job );
}

void JobSystem::ScheduleBatchJobs()
{
    //FlushBatchJobs();
    FlushBatchJobsLocally();
}
