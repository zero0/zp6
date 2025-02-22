//
// Created by phosg on 7/28/2023.
//

#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Math.h"
#include "Core/Job.h"
#include "Core/Vector.h"
#include "Core/Atomic.h"
#include "Core/Allocator.h"
#include "Core/Profiler.h"
#include "Core/Log.h"
#include "Core/String.h"
#include "Core/Function.h"

#include "Platform/Platform.h"

#define USE_JOB_STATE_TRACKING  ZP_DEBUG

namespace zp
{
    namespace
    {
        enum class JobState
        {
            Idle,
            Allocated,
            Prepared,
            Queued,
            Executing,
            Finished,
        };

        enum
        {
            kJobDataSize = 64,

            kJobsPerThread = 128,
            kJobsPerThreadMask = kJobsPerThread - 1,

            kJobQueueCount = 64,
            kJobQueueCountMask = kJobQueueCount - 1,
        };

        ZP_STATIC_ASSERT( zp_is_pow2( kJobQueueCount ) );
        ZP_STATIC_ASSERT( zp_is_pow2( kJobsPerThread ) );
    };

    struct Job
    {
        Job* parentJob;
        Job* nextJob;
        JobWorkFunc callback;
        zp_size_t jobSharedMemorySize;
        zp_uint32_t uncompletedJobs;
        zp_uint32_t batchId;
        zp_uint32_t jobStart;
        zp_uint32_t jobEnd;
#if USE_JOB_STATE_TRACKING
        JobState state;
#endif // USE_JOB_STATE_TRACKING
        FixedArray<zp_uint8_t, kJobDataSize> data;
    };

#pragma region JobQueue
    namespace
    {
        class JobQueue
        {
        public:
            JobQueue() = default;

            [[nodiscard]] zp_bool_t empty() const;

            void pushBack( Job* job );

            Job* popBack();

            Job* stealPopFront();

            void clear();

        private:
            zp_size_t m_back;
            zp_size_t m_front;
            FixedArray<Job*, kJobQueueCount> m_jobs;
        };

        //
        //
        //

        zp_bool_t JobQueue::empty() const
        {
            return m_back == m_front;
        }

        void JobQueue::pushBack( Job* job )
        {
            const zp_size_t back = m_back;
            m_jobs[ back & kJobQueueCountMask ] = job;
            Atomic::ExchangeSizeT( &m_back, back + 1 );
        }

        Job* JobQueue::popBack()
        {
            const zp_size_t back = m_back - 1;
            Atomic::ExchangeSizeT( &m_back, back );

            const zp_size_t front = m_front;

            Job* job = nullptr;

            if( front <= back )
            {
                job = m_jobs[ back & kJobQueueCountMask ];
                if( front != back )
                {
                    return job;
                }

                if( Atomic::CompareExchangeSizeT( &m_front, front + 1, front ) != front )
                {
                    job = nullptr;
                }

                m_back = front + 1;
            }
            else
            {
                m_back = front;
            }

            return job;
        }

        Job* JobQueue::stealPopFront()
        {
            const zp_size_t front = m_front;

            Atomic::MemoryBarrier();

            const zp_size_t back = m_back;

            Job* job = nullptr;

            if( front < back )
            {
                job = m_jobs[ front & kJobQueueCountMask ];

                if( Atomic::CompareExchangeSizeT( &m_front, front + 1, front ) != front )
                {
                    job = nullptr;
                }
            }

            return job;
        }

        void JobQueue::clear()
        {
            m_front = 0;
            m_back = 0;
        }
    }
#pragma endregion

    namespace
    {
        struct JobThreadInfo
        {
            FixedArray<Job, kJobsPerThread> jobs;
            zp_size_t allocatedJobCount;
            JobQueue* localJobQueue;
            JobQueue* localBatchJobQueue;
            zp_uint32_t threadId;
            CriticalSection lock;
        };

        thread_local JobThreadInfo t_threadInfo;

        //
        //
        //

        struct JobSystemContext
        {
            Vector<JobQueue> allJobQueues;
            Vector<JobQueue> allBatchJobQueues;
            Vector<ThreadHandle> allWorkerThreadHandles;
            zp_size_t stealJobQueueIndex;
            zp_size_t nextJobQueueIndex;
            ConditionVariable wakeCondition;
            zp_uint32_t threadCount;
            zp_int32_t isRunning;
        };

        JobSystemContext g_context;

        //
        //
        //

        void InitializeLocalThreadInfo( zp_size_t index )
        {
            JobThreadInfo& info = t_threadInfo;
            info = {};

            info.allocatedJobCount = 0;
            info.localJobQueue = &g_context.allJobQueues[ index ];
            info.localBatchJobQueue = &g_context.allBatchJobQueues[ index ];
            info.threadId = Platform::GetCurrentThreadId();
            info.lock = Platform::CreateCriticalSection();
        }

        void DestroyLocalThreadInfo()
        {
            JobThreadInfo& info = t_threadInfo;

            info.allocatedJobCount = 0;
            info.localJobQueue = nullptr;
            info.localBatchJobQueue = nullptr;
            Platform::CloseCriticalSection( info.lock );
        }

        JobQueue* GetLocalJobQueue()
        {
            return t_threadInfo.localJobQueue;
        }

        JobQueue* GetLocalBatchJobQueue()
        {
            return t_threadInfo.localBatchJobQueue;
        }

        JobQueue* GetStealJobQueue()
        {
            const zp_size_t index = Atomic::IncrementSizeT( &g_context.stealJobQueueIndex ) - 1;
            return &g_context.allJobQueues[ index % g_context.allJobQueues.length() ];
        }

        JobQueue* GetNextJobQueue()
        {
            const zp_size_t index = Atomic::IncrementSizeT( &g_context.nextJobQueueIndex ) - 1;
            return &g_context.allJobQueues[ index % g_context.allJobQueues.length() ];
        }

        Job* AllocateJob()
        {
            Job* job {};

            auto* info = &t_threadInfo;

            const zp_size_t index = t_threadInfo.allocatedJobCount % kJobsPerThread;
            ++t_threadInfo.allocatedJobCount;

            job = &t_threadInfo.jobs[ index ];

#if USE_JOB_STATE_TRACKING
            ZP_ASSERT( job->state == JobState::Idle );
            job->state = JobState::Allocated;
#endif // USE_JOB_STATE_TRACKING

            job->parentJob = nullptr;
            job->nextJob = nullptr;
            job->callback = nullptr;
            job->jobSharedMemorySize = 0;
            job->uncompletedJobs = 1;
            job->batchId = 0;
            job->jobStart = 0;
            job->jobEnd = 1;
            zp_zero_memory_array( job->data.data(), job->data.length() );

            return job;
        }

        void FreeJob( Job* job )
        {
#if USE_JOB_STATE_TRACKING
            ZP_ASSERT( job->state == JobState::Finished );
            job->state = JobState::Idle;
#endif // USE_JOB_STATE_TRACKING
        }

        void PrepareLocalJob( Job* job )
        {
#if USE_JOB_STATE_TRACKING
            job->state = JobState::Prepared;
#endif // USE_JOB_STATE_TRACKING
            GetLocalBatchJobQueue()->pushBack( job );
        }

        void QueueLocalJob( Job* job )
        {
#if USE_JOB_STATE_TRACKING
            job->state = JobState::Queued;
#endif // USE_JOB_STATE_TRACKING
            GetLocalJobQueue()->pushBack( job );
        }

        void QueueNextJob( Job* job )
        {
#if USE_JOB_STATE_TRACKING
            job->state = JobState::Queued;
#endif // USE_JOB_STATE_TRACKING
            GetNextJobQueue()->pushBack( job );
        }

        void AddJobDependency( Job* job, Job* dependency )
        {
            ZP_ASSERT( job->nextJob == nullptr );

            // push front
            job->nextJob = dependency->nextJob;
            dependency->nextJob = job;
        }

        void SetParentJob( Job* job, Job* parentJob )
        {
            ZP_ASSERT( job->parentJob == nullptr );
            job->parentJob = parentJob;

            Atomic::Increment( &parentJob->uncompletedJobs );
        }

        void FinishJob( Job* job )
        {
            const zp_size_t unfinishedJobs = Atomic::Decrement( &job->uncompletedJobs );

            if( unfinishedJobs == 0 )
            {
#if USE_JOB_STATE_TRACKING
                job->state = JobState::Finished;
#endif // USE_JOB_STATE_TRACKING

                Job* parent = job->parentJob;
                if( parent != nullptr )
                {
                    FinishJob( parent );
                }

                if( job->nextJob != nullptr )
                {
                    Job* dep = job->nextJob;
                    while( dep != nullptr )
                    {
                        QueueNextJob( dep );

                        dep = dep->nextJob;
                    }

                    Platform::NotifyAllConditionVariable( g_context.wakeCondition );
                }

                FreeJob( job );
            }
        }

        void ExecuteJob( Job* job )
        {
#if USE_JOB_STATE_TRACKING
            job->state = JobState::Executing;
#endif // USE_JOB_STATE_TRACKING

            if( job->callback )
            {
                Memory sharedMemory {
                    .ptr = job->jobSharedMemorySize == 0 ? nullptr : ZP_MALLOC( MemoryLabels::ThreadSafe, job->jobSharedMemorySize ),
                    .size = job->jobSharedMemorySize
                };

                JobWorkArgs args {
                    .currentJob { .job = job },
                    .parentJob { .job = job->parentJob },
                    .groupId = job->batchId,
                    .jobMemory { .ptr = job->data.data(), .size = job->data.length() },
                    .sharedMemory = sharedMemory,
                };

                for( zp_size_t offset = job->jobStart, end = job->jobEnd; offset < end; ++offset )
                {
                    args.index = offset;
                    job->callback( args );
                }

                if( sharedMemory.ptr != nullptr )
                {
                    ZP_FREE( MemoryLabels::ThreadSafe, sharedMemory.ptr );
                }
            }

            FinishJob( job );
        }

        Job* RequestQueuedJob()
        {
            Job* job = nullptr;

            JobQueue* jobQueue = GetLocalJobQueue();
            if( jobQueue != nullptr && !jobQueue->empty() )
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
            }
#if USE_JOB_STATE_TRACKING
            else
            {
                ZP_ASSERT( job->state == JobState::Queued );
            }
#endif // USE_JOB_STATE_TRACKING

            return job;
        }

        void FlushBatchJobs()
        {
            JobQueue* batchQueue = GetLocalBatchJobQueue();

            while( !batchQueue->empty() )
            {
                Job* job = batchQueue->popBack();
                QueueNextJob( job );
            }
        }

        void FlushBatchJobsLocally()
        {
            JobQueue* batchQueue = GetLocalBatchJobQueue();

            while( !batchQueue->empty() )
            {
                Job* job = batchQueue->popBack();
                QueueLocalJob( job );
            }
        }

        zp_bool_t IsJobComplete( Job* job )
        {
            return job == nullptr || job->uncompletedJobs == 0;
        }

        void WaitForJobComplete( Job* job )
        {
            while( !IsJobComplete( job ) )
            {
                Job* waitJob = RequestQueuedJob();
                if( waitJob != nullptr )
                {
                    ExecuteJob( waitJob );
                }
            }
        }

        zp_uint32_t WorkerThreadFunc( void* threadData )
        {
#if ZP_USE_PROFILER
            Profiler::InitializeProfilerThread();
#endif

            const zp_size_t index = static_cast<zp_size_t>( reinterpret_cast<zp_ptr_t>( threadData ) );

            Log::info() << "Entering Worker Thread..." << Log::endl;

            InitializeLocalThreadInfo( index );

            while( Atomic::And( &g_context.isRunning, 1 ) == 1 )
            {
                Job* job = RequestQueuedJob();

                while( job != nullptr )
                {
                    ExecuteJob( job );

                    job = RequestQueuedJob();
                }

                Platform::EnterCriticalSection( t_threadInfo.lock );
                Platform::WaitConditionVariable( g_context.wakeCondition, t_threadInfo.lock );
                Platform::LeaveCriticalSection( t_threadInfo.lock );
            }

            Log::info() << "Exiting Worker Thread" << Log::endl;

            DestroyLocalThreadInfo();

            return 0;
        }
    };

    void JobSystem::Setup( MemoryLabel memoryLabel, zp_uint32_t threadCount )
    {
        const zp_size_t jobQueueCount = threadCount + 1;

        g_context.allJobQueues = Vector<JobQueue>( jobQueueCount, memoryLabel );
        g_context.allBatchJobQueues = Vector<JobQueue>( jobQueueCount, memoryLabel );
        g_context.allWorkerThreadHandles = Vector<ThreadHandle>( threadCount, memoryLabel );
        g_context.stealJobQueueIndex = 0;
        g_context.nextJobQueueIndex = 0;
        g_context.wakeCondition = Platform::CreateConditionVariable();
        g_context.threadCount = threadCount;
        g_context.isRunning = 0;
    }

    void JobSystem::Teardown()
    {
        ExitJobThreads();

        g_context.allJobQueues.destroy();
        g_context.allBatchJobQueues.destroy();
        g_context.allWorkerThreadHandles.destroy();
    }

    void JobSystem::InitializeJobThreads()
    {
        g_context.allJobQueues.reset();
        g_context.allBatchJobQueues.reset();
        g_context.allWorkerThreadHandles.reset();

        // mark as running
        g_context.isRunning = 1;

        const zp_size_t stackSize = 1 MB;
        MutableFixedString64 threadName;

        const zp_uint32_t numAvailableProcessors = Platform::GetProcessorCount() - 1;
        for( zp_uint32_t i = 0; i < g_context.threadCount; ++i )
        {
            // thread queues
            g_context.allJobQueues.pushBackEmpty();
            g_context.allBatchJobQueues.pushBackEmpty();

            zp_uint32_t threadID;
            const ThreadHandle threadHandle = Platform::CreateThread( WorkerThreadFunc, reinterpret_cast<void*>( static_cast<zp_ptr_t>( i ) ), stackSize, &threadID );

            Platform::SetThreadIdealProcessor( threadHandle, 1 + ( i % numAvailableProcessors ) ); // proc 0 is used for main thread

            threadName.format( "JobThread-%d %d", threadID, i );

            Platform::SetThreadName( threadHandle, threadName );

            g_context.allWorkerThreadHandles[ i ] = threadHandle;
        }

        // main thread queues
        g_context.allJobQueues.pushBackEmpty();
        g_context.allBatchJobQueues.pushBackEmpty();

        // setup main thread job info
        InitializeLocalThreadInfo( g_context.threadCount );
    }

    void JobSystem::ExitJobThreads()
    {
        g_context.isRunning = 0;

        // notify all worker threads
        Platform::NotifyAllConditionVariable( g_context.wakeCondition );

        // wait for all threads to finish
        Platform::JoinThreads( g_context.allWorkerThreadHandles.data(), g_context.allWorkerThreadHandles.length() );

        // close threads
        for( auto thread : g_context.allWorkerThreadHandles )
        {
            Platform::CloseThread( thread );
        }

        // destroy main thread job info
        DestroyLocalThreadInfo();
    }

    void JobSystem::Complete( JobHandle jobHandle )
    {
        WaitForJobComplete( jobHandle.job );
    }

    zp_bool_t JobSystem::IsComplete( JobHandle jobHandle )
    {
        return IsJobComplete( jobHandle.job );
    }

    JobHandle JobSystem::Execute( JobWorkFunc func )
    {
        Job* job = AllocateJob();

        job->callback = func;

        QueueNextJob( job );

        Platform::NotifyOneConditionVariable( g_context.wakeCondition );

        return { .job = job };
    }

    JobHandle JobSystem::PrepareEmpty()
    {
        Job* job = AllocateJob();

        PrepareLocalJob( job );

        return { .job = job };
    }

    JobHandle JobSystem::Prepare( JobWorkFunc func, JobHandle dependency )
    {
        Job* job = AllocateJob();

        job->callback = func;

        if( dependency.job == nullptr )
        {
            PrepareLocalJob( job );
        }
        else
        {
#if USE_JOB_STATE_TRACKING
            ZP_ASSERT( dependency.job->state == JobState::Prepared || dependency.job->state == JobState::Allocated );
#endif // USE_JOB_STATE_TRACKING
            AddJobDependency( job, dependency.job );
        }

        return { .job = job };
    }

    JobHandle Dispatch( zp_size_t length, zp_size_t batchCount, JobWorkFunc func )
    {
        ZP_ASSERT( length > 0 );
        ZP_ASSERT( batchCount > 0 );

        const zp_size_t jobCount = ( length + batchCount ) / batchCount;

        Job* parentJob = AllocateJob();

        zp_size_t offset = 0;
        for( zp_size_t batch = 0; batch < jobCount; ++batch )
        {
            Job* job = AllocateJob();

            job->callback = func;

            job->batchId = batch;
            job->jobStart = offset;

            offset = zp_min( length, offset + batchCount );

            job->jobEnd = offset;

            SetParentJob( job, parentJob );

            QueueNextJob( job );
        }

        QueueNextJob( parentJob );

        Platform::NotifyAllConditionVariable( g_context.wakeCondition );

        return { .job = parentJob };
    }

    JobHandle JobSystem::PrepareDispatch( zp_size_t length, zp_size_t batchCount, JobWorkFunc func, JobHandle dependency )
    {
        ZP_ASSERT( length > 0 );
        ZP_ASSERT( batchCount > 0 );

        const zp_size_t jobCount = zp_divide_round_up( length, batchCount );

        Job* parentJob = AllocateJob();

        zp_size_t offset = 0;
        for( zp_size_t batch = 0; batch < jobCount; ++batch )
        {
            Job* job = AllocateJob();

            job->callback = func;

            job->batchId = batch;
            job->jobStart = offset;

            offset = zp_min( length, offset + batchCount );

            job->jobEnd = offset;

            SetParentJob( job, parentJob );

            PrepareLocalJob( job );
        }

        if( dependency.job == nullptr )
        {
            PrepareLocalJob( parentJob );
        }
        else
        {
#if USE_JOB_STATE_TRACKING
            ZP_ASSERT( dependency.job->state == JobState::Prepared || dependency.job->state == JobState::Allocated );
#endif // USE_JOB_STATE_TRACKING
            AddJobDependency( parentJob, dependency.job );
        }

        return { .job = parentJob };
    }

    JobHandle JobSystem::Start( Memory jobData, JobCallback jobCallback )
    {
        Job* job = AllocateJob();

        job->callback = jobCallback;
        zp_memcpy( job->data.data(), job->data.length(), jobData.ptr, jobData.size );

        QueueNextJob( job );

        Platform::NotifyOneConditionVariable( g_context.wakeCondition );

        return { .job = job };
    }


    JobHandle JobSystem::Run( Memory jobData, JobCallback jobCallback )
    {
        Job* job = AllocateJob();

        job->callback = jobCallback;
        zp_memcpy( job->data.data(), job->data.length(), jobData.ptr, jobData.size );

        ExecuteJob( job );

        return {};
    }

    void JobSystem::ScheduleBatchJobs()
    {
        FlushBatchJobs();

        Platform::NotifyAllConditionVariable( g_context.wakeCondition );
    }

    void JobSystem::ProcessJobs()
    {
        Job* job = RequestQueuedJob();

        while( job != nullptr )
        {
            ExecuteJob( job );

            job = RequestQueuedJob();
        }
    }
};

#if 0
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

        void clear();

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

    void JobQueue::clear()
    {
        m_front = 0;
        m_back = 0;
    }
}

namespace
{
    struct JobThreadInfo
    {
        Job jobs[kJobsPerThread];
        zp_size_t allocatedJobCount;
        JobQueue* localJobQueue;
        JobQueue* localBatchJobQueue;
        zp_bool_t isRunning;
    };
    thread_local JobThreadInfo t_threadInfo;


    MemoryArray<JobQueue> g_allQueues;

    MemoryArray<JobQueue*> g_allJobQueues;

    MemoryArray<JobQueue*> g_allBatchJobQueues;

    MemoryArray<JobThreadInfo*> g_allJobThreadInfos;

    MemoryArray<ThreadHandle> g_allJobThreadHandles;

    zp_size_t g_stealJobQueueIndex;

    MemoryLabel g_memoryLabel;

    //
    //
    //

    JobQueue* GetLocalJobQueue()
    {
        return t_threadInfo.localJobQueue;
    }

    JobQueue* GetLocalBatchJobQueue()
    {
        return t_threadInfo.localBatchJobQueue;
    }

    JobQueue* GetStealJobQueue()
    {
        const zp_size_t index = Atomic::IncrementSizeT( &g_stealJobQueueIndex ) - 1;
        return g_allJobQueues[ index % g_allJobQueues.length ];
    }

    Job* AllocateJob()
    {
        const zp_size_t index = ++t_threadInfo.allocatedJobCount;
        return &t_threadInfo.jobs[ index & kJobsPerThreadMask ];
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
            Platform::YieldCurrentThread();
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
        t_threadInfo.isRunning = false;
    }

    zp_uint32_t WorkerThreadFunc( void* threadData )
    {
#if ZP_USE_PROFILER
        Profiler::InitializeProfilerThread();
#endif

        const zp_size_t index = static_cast<zp_size_t>( reinterpret_cast<zp_ptr_t>( threadData ) );

        g_allJobThreadInfos[ index ] = &t_threadInfo;

        zp_zero_memory_array( t_threadInfo.jobs );

        t_threadInfo.allocatedJobCount = 0;
        t_threadInfo.localJobQueue = g_allJobQueues[ index ];
        t_threadInfo.localBatchJobQueue = g_allBatchJobQueues[ index ];
        t_threadInfo.isRunning = true;

        while( t_threadInfo.isRunning )
        {
            Job* job = GetJob();
            if( job && t_threadInfo.isRunning )
            {
                ExecuteJob( job );
            }

            Platform::YieldCurrentThread();
        }

        zp_printfln( "Exiting Worker Thread %d", Platform::GetCurrentThreadId() );

        t_threadInfo.allocatedJobCount = 0;
        t_threadInfo.localBatchJobQueue = nullptr;
        t_threadInfo.localJobQueue = nullptr;

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

PrepariedJob::PrepariedJob( Job* job )
    : m_job( job )
{
}

JobHandle PrepariedJob::schedule()
{
    Job* job = m_job;
    m_job = nullptr;

    if( job )
    {
        GetLocalBatchJobQueue()->pushBack( job );
    }

    return JobHandle( job );
}

JobHandle PrepariedJob::schedule( JobHandle dependency )
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

    const zp_uint32_t threadJobCount = threadCount + 1;

    g_allQueues = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobQueue, threadJobCount * 2 ),
        .length = threadJobCount * 2
    };
    zp_zero_memory_array( g_allQueues.ptr, g_allQueues.length );

    g_allJobQueues = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobQueue*, threadJobCount ),
        .length = threadJobCount
    };

    g_allBatchJobQueues = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobQueue*, threadJobCount ),
        .length = threadJobCount
    };

    for( zp_uint32_t i = 0; i < threadJobCount; ++i )
    {
        g_allJobQueues[ i ] = &g_allQueues[ i ];
        g_allBatchJobQueues[ i ] = &g_allQueues[ i + threadJobCount ];
    }

    g_allJobThreadInfos = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, JobThreadInfo*, threadJobCount ),
        .length = threadJobCount
    };
    zp_zero_memory_array( g_allJobThreadInfos.ptr, g_allJobThreadInfos.length );

    g_allJobThreadHandles = {
        .ptr = ZP_MALLOC_T_ARRAY( memoryLabel, ThreadHandle, threadCount ),
        .length = threadCount
    };
    zp_zero_memory_array( g_allJobThreadHandles.ptr, g_allJobThreadHandles.length );
}

void JobSystem::Teardown()
{
    ZP_FREE( g_memoryLabel, g_allJobThreadHandles.ptr );
    ZP_FREE( g_memoryLabel, g_allJobThreadInfos.ptr );

    ZP_FREE( g_memoryLabel, g_allBatchJobQueues.ptr );
    ZP_FREE( g_memoryLabel, g_allJobQueues.ptr );
    ZP_FREE( g_memoryLabel, g_allQueues.ptr );
}

void JobSystem::InitializeJobThreads()
{
    const zp_size_t stackSize = 1 MB;
    MutableFixedString<64> threadName;

    const zp_uint32_t numAvailableProcessors = Platform::GetProcessorCount() - 1;
    for( zp_uint32_t i = 0; i < g_allJobThreadHandles.length; ++i )
    {
        zp_uint32_t threadID;
        ThreadHandle threadHandle = Platform::CreateThread( WorkerThreadFunc, reinterpret_cast<void*>( static_cast<zp_ptr_t>( i ) ), stackSize, &threadID );

        Platform::SetThreadIdealProcessor( threadHandle, 1 + ( i % numAvailableProcessors ) ); // proc 0 is used for main thread

        threadName.format( "JobThread-%d %d", threadID, i );

        Platform::SetThreadName( threadHandle, threadName );

        g_allJobThreadHandles[ i ] = threadHandle;
    }

    // setup main thread job info
    zp_zero_memory_array( t_threadInfo.jobs );

    g_allJobThreadInfos[ g_allJobThreadHandles.length ] = &t_threadInfo;

    t_threadInfo.allocatedJobCount = 0;
    t_threadInfo.localJobQueue = g_allJobQueues[ g_allJobThreadHandles.length ];
    t_threadInfo.localBatchJobQueue = g_allBatchJobQueues[ g_allJobThreadHandles.length ];
    t_threadInfo.isRunning = true;
}

void JobSystem::ExitJobThreads()
{
    // shutdown job worker threads
    for( zp_size_t i = 0; i < g_allJobThreadInfos.length; ++i )
    {
        g_allJobThreadInfos[ i ]->isRunning = false;
    }

    // wait for all threads to finish
    Platform::JoinThreads( g_allJobThreadHandles.ptr, g_allJobThreadHandles.length );

    // close threads
    for( zp_size_t i = 0; i < g_allJobThreadHandles.length; ++i )
    {
        Platform::CloseThread( g_allJobThreadHandles[ i ] );
        g_allJobThreadHandles[ i ] = {};
    }

    // clear info pointers
    zp_zero_memory_array( g_allJobThreadInfos.ptr, g_allJobThreadInfos.length );

    // clear queues
    for( zp_size_t i = 0; i < g_allQueues.length; ++i )
    {
        g_allQueues[ i ].clear();
    }

    // clear main thread job info
    t_threadInfo.allocatedJobCount = 0;
    t_threadInfo.localJobQueue = nullptr;
    t_threadInfo.localBatchJobQueue = nullptr;
    t_threadInfo.isRunning = false;
}

void JobSystem::ProcessJobs()
{
    Job* job = GetJob();
    if( job )
    {
        ExecuteJob( job );
    }
}

PrepariedJob JobSystem::Start()
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

    return PrepariedJob( job );
}

PrepariedJob JobSystem::Start( JobHandle parentJob )
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

    return PrepariedJob( job );
}

PrepariedJob JobSystem::Start( JobWorkFunc execFunc )
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

    return PrepariedJob( job );
}

PrepariedJob JobSystem::Start( JobWorkFunc execFunc, JobHandle parentJob )
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

    return PrepariedJob( job );
}

PrepariedJob JobSystem::Start( JobWorkFunc execFunc, void* data, zp_size_t size )
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

    return PrepariedJob( job );
}

PrepariedJob JobSystem::Start( JobWorkFunc execFunc, void* data, zp_size_t size, JobHandle parentJob )
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

    return PrepariedJob( job );
}

void JobSystem::ScheduleBatchJobs()
{
    //FlushBatchJobs();
    FlushBatchJobsLocally();
}
#endif
