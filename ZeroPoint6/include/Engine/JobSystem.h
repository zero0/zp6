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

        JobHandle Schedule( const PreparedJobHandle& preparedJobHandle );

        //
        //
        //

        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc )
        {
            Job* job = PrepareGenericJob( jobWorkFunc, nullptr, 0, {}, nullptr );
            return PreparedJobHandle( job );
        }

        template<typename T>
        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const T& data )
        {
            Job* job = PrepareGenericJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), {}, nullptr );
            return PreparedJobHandle( job );
        }

        template<typename T>
        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const T& data, const PreparedJobHandle& dependency )
        {
            Job* job = PrepareGenericJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency, nullptr );
            return PreparedJobHandle( job );
        }

        template<typename T>
        PreparedJobHandle PrepareJob( JobWorkFunc jobWorkFunc, const T& data, const PreparedJobHandle& dependency, const char* debugName )
        {
            Job* job = PrepareGenericJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), dependency, debugName );
            return PreparedJobHandle( job );
        }

        //
        //
        //

        template<typename T>
        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const T& data )
        {
            Job* job = PrepareGenericChildJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), parentJobHandle, {}, nullptr );
            return PreparedJobHandle( job );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJob( const PreparedJobHandle& parentJobHandle, JobWorkFunc jobWorkFunc, const T& data, const PreparedJobHandle& dependency )
        {
            Job* job = PrepareGenericChildJob( jobWorkFunc, static_cast<const void*>(&data), sizeof( T ), parentJobHandle, dependency, nullptr );
            return PreparedJobHandle( job );
        }

        //
        //
        //

        template<typename T>
        PreparedJobHandle PrepareJobData( T** data )
        {
            Job* job = PrepareGenericJob( Wrapper<T>::ExecuteWrapper, nullptr, sizeof( Wrapper < T > ), {}, nullptr );

            auto wrapperPtr = static_cast<Wrapper <T>*>( job->data());
            wrapperPtr->func = T::Execute;
            *data = &wrapperPtr->data;

            return PreparedJobHandle( job );
        }

        //
        //
        //

        template<typename T>
        PreparedJobHandle PrepareJobData( const T& data )
        {
            Wrapper <T> wrapper { .func = T::Execute, .data = data };
            return PrepareJob( Wrapper<T>::ExecuteWrapper, wrapper, {}, GetDebugName<T>() );
        }

        template<typename T>
        PreparedJobHandle PrepareJobData( const T& data, const PreparedJobHandle& dependency )
        {
            Wrapper <T> wrapper { .func = T::Execute, .data = data };
            return PrepareJob( Wrapper<T>::ExecuteWrapper, wrapper, dependency, GetDebugName<T>() );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJobData( const PreparedJobHandle& parentJobHandle, const T& data )
        {
            Wrapper <T> wrapper { .func = T::Execute, .data = data };
            return PrepareChildJob( parentJobHandle, Wrapper<T>::ExecuteWrapper, wrapper );
        }

        template<typename T>
        PreparedJobHandle PrepareChildJobData( const PreparedJobHandle& parentJobHandle, const T& data, const PreparedJobHandle& dependency )
        {
            Wrapper <T> wrapper { .func = T::Execute, .data = data };
            return PrepareChildJob( parentJobHandle, Wrapper<T>::ExecuteWrapper, wrapper, dependency );
        }

        //
        //
        //

    private:
        static Job* PrepareGenericJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& dependency, const char* name );

        static Job* PrepareGenericChildJob( JobWorkFunc jobWorkFunc, const void* jobData, zp_size_t size, const PreparedJobHandle& parent, const PreparedJobHandle& dependency, const char* name );

        template<typename T>
        struct Wrapper
        {
            typedef void (* WrapperFunc)( const JobHandle& parentJobHandle, const T* ptr );

            WrapperFunc func;
            T data;

            static void ExecuteWrapper( Job* job, void* data )
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
