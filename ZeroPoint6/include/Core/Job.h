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

        static JobData Start();

        static JobData Start( JobHandle parentJob );

        static JobData Start( JobWorkFunc execFunc );

        static JobData Start( JobWorkFunc execFunc, JobHandle parentJob );

        static void ScheduleBatchJobs();
    };
}

#endif //ZP_JOB_H
