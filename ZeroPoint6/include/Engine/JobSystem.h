//
// Created by phosg on 11/10/2021.
//

#ifndef ZP_JOBSYSTEM_H
#define ZP_JOBSYSTEM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

namespace zp
{
    struct Job;

    typedef void (* JobWorkFunc)( Job* job, void* data );

    enum : zp_size_t
    {
        kJobTotalSize = 4 KB,
        kJobPayloadSize = kJobTotalSize - ( sizeof( JobWorkFunc* ) + sizeof( Job* ) + sizeof( Job* ) + sizeof( zp_size_t ) ),
#if ZP_DEBUG
        kJobDataDebugSize = 64,
#else
        kJobDataDebugSize = 0,
#endif
        kJobDataOffset = kJobDataDebugSize,
        kJobDataSize = kJobPayloadSize - kJobDataOffset
    };

#if ZP_DEBUG
#define ZP_JOB_DEBUG_NAME( t )    static const char* DebugName() { return #t; }
#else
#define ZP_JOB_DEBUG_NAME(...)
#endif

    namespace
    {
        template<typename T>
        ZP_FORCEINLINE const char* GetDebugName()
        {
#if ZP_DEBUG
            return T::DebugName();
#else
            return nullptr;
#endif
        }
    }

    struct Job
    {
        JobWorkFunc func;
        Job* parent;
        Job* next;
        zp_size_t unfinishedJobs;
        zp_uint8_t payload[kJobPayloadSize];

        ZP_FORCEINLINE void* data()
        {
            return payload + kJobDataOffset;
        }

        ZP_FORCEINLINE const char* name()
        {
#if ZP_DEBUG
            return reinterpret_cast<const char*>( payload );
#else
            return nullptr;
#endif
        }

        ZP_FORCEINLINE void set_name( const char* debugName )
        {
#if ZP_DEBUG
            if( debugName )
            {
                zp_strcpy( debugName, reinterpret_cast<char*>( payload ), kJobDataDebugSize );
            }
#endif
        }
    };

    ZP_STATIC_ASSERT( sizeof( Job ) == kJobTotalSize );

    //
    //
    //

    class JobSystem;

    class PreparedJobHandle
    {
    public:
        PreparedJobHandle();

    private:
        explicit PreparedJobHandle( Job* job );

        Job* m_job;

        friend class JobSystem;
    };

    class JobHandle
    {
    public:
        JobHandle();

    private:
        explicit JobHandle( Job* job );

    public:
        [[nodiscard]] zp_bool_t isDone() const;

        void complete();

    private:
        Job* m_job;

        friend class JobSystem;
    };

    //
    //
    //

    class JobQueue
    {
    ZP_NONCOPYABLE( JobQueue );

    public:
        JobQueue( MemoryLabel memoryLabel, zp_size_t count );

        ~JobQueue();

        void push( Job* job );

        Job* pop();

        Job* steal();

        [[nodiscard]] zp_bool_t isEmpty() const;

        [[nodiscard]] zp_size_t count() const;

        [[nodiscard]] zp_bool_t canSteal() const;

        void setCanSteal( zp_bool_t canSteal );

    private:
        Job** m_jobs;
        zp_size_t m_capacity;
        const zp_size_t m_mask;

        zp_size_t m_bottom;
        zp_size_t m_top;
        ZP_BOOL32( m_canSteal );

    public:
        const MemoryLabel memoryLabel;
    };

    //
    //
    //

    class JobSystem
    {
    ZP_NONCOPYABLE( JobSystem )

    public:
        static JobSystem* GetJobSystem();

    public:
        JobSystem( MemoryLabel memoryLabel, zp_size_t threadCount );

        ~JobSystem();

        void ExitJobThreads();

        //
        //
        //

        PreparedJobHandle Prepare( const JobHandle& jobHandle ) const;

        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc );

        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const PreparedJobHandle& dependency );

        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size );

        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& dependency );

        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& dependency, const char* name );

        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc );

        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const PreparedJobHandle& dependency );

        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size );

        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& dependency );

        JobHandle Schedule( const PreparedJobHandle& preparedJobHandle );

        //
        //
        //

        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc );

        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size );

        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const JobHandle& dependency );

        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const JobHandle& dependency );

        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const JobHandle& dependency, const char* debugName );

        JobHandle ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc );

        JobHandle ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size );

        JobHandle ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const JobHandle& dependency );

        JobHandle ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const JobHandle& dependency );

        //
        //
        //

        template<typename T>
        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const T& data )
        {
            return PrepareJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ) );
        }

        template<typename T>
        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const T& data, const PreparedJobHandle& dependency )
        {
            return PrepareJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency );
        }

        template<typename T>
        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const T& data, const PreparedJobHandle& dependency, const char* debugName )
        {
            return PrepareJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency, debugName );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const T& data )
        {
            return PrepareChildJob( parentJobHandle, jobWorkFunc, static_cast<const void*>(&data), sizeof( T ) );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const T& data, const PreparedJobHandle& dependency )
        {
            return PrepareChildJob( parentJobHandle, jobWorkFunc, static_cast<const void*>( &data ), sizeof( T ), dependency );
        }

        //
        //
        //

        template<typename T>
        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const T& data )
        {
            return ScheduleJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ) );
        }

        template<typename T>
        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const T& data, const JobHandle& dependency )
        {
            return ScheduleJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency );
        }

        template<typename T>
        JobHandle ScheduleJob( JobWorkFunc jobWorkFunc, const T& data, const JobHandle& dependency, const char* debugName )
        {
            return ScheduleJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency, debugName );
        }

        template<typename T>
        JobHandle ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const T& data )
        {
            return ScheduleChildJob( parentJobHandle, jobWorkFunc, static_cast<const void*>(&data), sizeof( T ) );
        }

        template<typename T>
        JobHandle ScheduleChildJob( const JobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const T& data, const JobHandle& dependency )
        {
            return ScheduleChildJob( parentJobHandle, jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency );
        }

        //
        //
        //

        template<typename T>
        PreparedJobHandle PrepareJobData( const T& data )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return PrepareJob( Wrapper<T>::WrapperJob, wrapper, {}, GetDebugName<T>() );
        }

        template<typename T>
        PreparedJobHandle PrepareJobData( const T& data, const PreparedJobHandle& dependency )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return PrepareJob( Wrapper<T>::WrapperJob, wrapper, dependency, GetDebugName<T>() );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJobData( const PreparedJobHandle& parentJobHandle, const T& data )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return PrepareChildJob( parentJobHandle, Wrapper<T>::WrapperJob, wrapper );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJobData( const PreparedJobHandle& parentJobHandle, const T& data, const PreparedJobHandle& dependency )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return PrepareChildJob( parentJobHandle, Wrapper<T>::WrapperJob, wrapper, dependency );
        }

        //
        //
        //

        template<typename T>
        JobHandle ScheduleJobData( const T& data )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return ScheduleJob( Wrapper<T>::WrapperJob, wrapper, {}, GetDebugName<T>() );
        }

        template<typename T>
        JobHandle ScheduleJobData( const T& data, const JobHandle& dependency )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return ScheduleJob( Wrapper<T>::WrapperJob, wrapper, dependency, GetDebugName<T>() );
        }

        template<typename T>
        JobHandle ScheduleChildJobData( const JobHandle& parentJobHandle, const T& data )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return ScheduleChildJob( parentJobHandle, Wrapper<T>::WrapperJob, wrapper );
        }

        template<typename T>
        JobHandle ScheduleChildJobData( const JobHandle& parentJobHandle, const T& data, const JobHandle& dependency )
        {
            Wrapper <T> wrapper { T::Execute, zp_move( data ) };
            return ScheduleParentJob( parentJobHandle, Wrapper<T>::WrapperJob, wrapper, dependency );
        }

    private:
        template<typename T>
        struct Wrapper
        {
            typedef void (* WrapperFunc)( const JobHandle& parentJobHandle, const T* ptr );

            WrapperFunc func;
            T data;

            static void WrapperJob( Job* job, void* data )
            {
                auto ptr = static_cast<Wrapper <T>*>( data );
                if( ptr->func )
                {
                    ptr->func( JobHandle( job ), &ptr->data );
                }
            }
        };

        JobQueue** m_jobQueues;
        zp_handle_t* m_threadHandles;
        zp_size_t m_threadCount;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_JOBSYSTEM_H
