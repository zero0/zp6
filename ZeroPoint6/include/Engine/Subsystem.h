//
// Created by phosg on 3/25/2023.
//

#ifndef ZP_SUBSYSTEM_H
#define ZP_SUBSYSTEM_H

#include "Core/Vector.h"
#include "Core/Job.h"
#include "Engine/ExecutionGraph.h"

namespace zp
{
    ZP_PURE_INTERFACE ISubsystem
    {
    public:
        virtual void Initialize() {}

        virtual void Teardown() {}

        virtual JobHandle Process( const JobHandle& parentJob, const JobHandle& inputHandle ) { return inputHandle; }
    };

    struct Subsystem
    {
        ISubsystem* subsystem;
        zp_hash64_t id;
        zp_size_t refCount;
        zp_int32_t order;
        MemoryLabel memoryLabel;

        ZP_FORCEINLINE void AddRef()
        {
            ++refCount;
        }

        ZP_FORCEINLINE void RemoveRef()
        {
            ZP_ASSERT( refCount > 0 );
            --refCount;
        }
    };


    struct SubsystemHandleF
    {
    public:
        SubsystemHandleF( const SubsystemHandleF& handle )
            : m_subsystem( handle.m_subsystem )
        {
        }

        SubsystemHandleF( SubsystemHandleF&& handle ) noexcept
            : m_subsystem( handle.m_subsystem )
        {
            handle.m_subsystem = nullptr;
        }

        ~SubsystemHandleF()
        {
            if( m_subsystem )
            {
                --m_subsystem->refCount;
                m_subsystem = nullptr;
            }
        }

        [[nodiscard]] zp_bool_t IsValid() const
        {
            return m_subsystem != nullptr;
        }

        ISubsystem* operator->()
        {
            return m_subsystem->subsystem;
        }

        const ISubsystem* operator->() const
        {
            return m_subsystem->subsystem;
        }

        zp_bool_t operator==( const SubsystemHandleF& handle ) const
        {
            return m_subsystem == handle.m_subsystem;
        }

        zp_bool_t operator!=( const SubsystemHandleF& handle ) const
        {
            return m_subsystem != handle.m_subsystem;
        }

        SubsystemHandleF& operator=( const SubsystemHandleF& handle )
        {
            if( *this != handle )
            {
                if( m_subsystem )
                {
                    --m_subsystem->refCount;
                }

                m_subsystem = handle.m_subsystem;

                if( m_subsystem )
                {
                    ++m_subsystem->refCount;
                }
            }

            return *this;
        }

        SubsystemHandleF& operator=( SubsystemHandleF&& handle ) noexcept
        {
            if( *this != handle )
            {
                m_subsystem = handle.m_subsystem;
                handle.m_subsystem = nullptr;
            }

            return *this;
        }

    private:
        SubsystemHandleF()
            : m_subsystem( nullptr )
        {
        }

        explicit SubsystemHandleF( Subsystem* subsystem )
            : m_subsystem( subsystem )
        {
            if( m_subsystem )
            {
                ++m_subsystem->refCount;
            }
        }

        Subsystem* m_subsystem;

        friend class SubsystemManager;
    };

    template<typename T>
    struct SubsystemHandle
    {
    public:
        SubsystemHandle( const SubsystemHandle<T>& handle )
            : m_subsystem( handle.m_subsystem )
        {
        }

        SubsystemHandle( SubsystemHandle<T>&& handle ) noexcept
            : m_subsystem( handle.m_subsystem )
        {
            handle.m_subsystem = nullptr;
        }

        ~SubsystemHandle()
        {
            if( m_subsystem )
            {
                m_subsystem->RemoveRef();
                m_subsystem = nullptr;
            }
        }

        [[nodiscard]] zp_bool_t IsValid() const
        {
            return m_subsystem != nullptr;
        }

        T* operator->()
        {
            return static_cast<T*>( m_subsystem->subsystem );
        }

        const T* operator->() const
        {
            return static_cast<const T*>( m_subsystem->subsystem );
        }

        zp_bool_t operator==( const SubsystemHandle<T>& handle ) const
        {
            return m_subsystem == handle.m_subsystem;
        }

        zp_bool_t operator!=( const SubsystemHandle<T>& handle ) const
        {
            return m_subsystem != handle.m_subsystem;
        }

        SubsystemHandle<T>& operator=( const SubsystemHandle<T>& handle )
        {
            if( this != handle )
            {
                if( m_subsystem )
                {
                    m_subsystem->RemoveRef();
                }

                m_subsystem = handle.m_subsystem;

                if( m_subsystem )
                {
                    m_subsystem->AddRef();
                }
            }

            return *this;
        }

        SubsystemHandle<T>& operator=( SubsystemHandle<T>&& handle ) noexcept
        {
            if( this != handle )
            {
                m_subsystem = handle.m_subsystem;
                handle.m_subsystem = nullptr;
            }

            return *this;
        }

    private:
        SubsystemHandle()
            : m_subsystem( nullptr )
        {
        }

        explicit SubsystemHandle( Subsystem* subsystem )
            : m_subsystem( subsystem )
        {
            if( m_subsystem )
            {
                m_subsystem->AddRef();
            }
        }

        Subsystem* m_subsystem;

        friend class SubsystemManager;
    };

    class SubsystemManager
    {
    public:
        explicit SubsystemManager( MemoryLabel memoryLabel );

        template<typename T>
        void RegisterSubsystem( MemoryLabel label )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();

            RegisterSubsystem( {
                .subsystem = ZP_NEW( label, T ),
                .id = typeHash,
                .memoryLabel = label
            } );
        }

        template<typename T>
        void RequestSubsystem( SubsystemHandle<T>& handle )
        {
            const zp_hash64_t typeHash = zp_type_hash<T>();

            Subsystem* subsystem = FindSubsystem( typeHash );

            handle = SubsystemHandle<T>( subsystem );
        }

        void BuildExecutionGraph( ExecutionGraph& executionGraph );

    private:
        struct RegisterSubsystemDesc
        {
            ISubsystem* subsystem;
            zp_hash64_t id;
            MemoryLabel memoryLabel;
        };

        void RegisterSubsystem( const RegisterSubsystemDesc& desc );

        [[nodiscard]] Subsystem* FindSubsystem( zp_hash64_t id ) const;

        Vector<Subsystem*> m_subsystems;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_SUBSYSTEM_H
