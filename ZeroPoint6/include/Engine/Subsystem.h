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

    struct SubsystemInstance
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

    template<typename T>
    class SubsystemHandle
    {
    public:
        typedef SubsystemHandle<T> handle_type;
        typedef handle_type& ref_handle_type;
        typedef const handle_type& const_ref_handle_type;
        typedef handle_type&& move_handle_type;

        typedef zp_remove_reference_t<T> value_type;
        typedef value_type* ptr_value_type;
        typedef const value_type* const_ptr_value_type;

        SubsystemHandle( const_ref_handle_type handle )
            : m_subsystem( handle.m_subsystem )
        {
            if( m_subsystem )
            {
                m_subsystem->AddRef();
            }
        }

        SubsystemHandle( move_handle_type handle ) noexcept
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

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t IsValid() const
        {
            return m_subsystem != nullptr;
        }

        ZP_FORCEINLINE ptr_value_type operator->()
        {
            return static_cast<ptr_value_type>( m_subsystem->subsystem );
        }

        ZP_FORCEINLINE const_ptr_value_type operator->() const
        {
            return static_cast<const_ptr_value_type>( m_subsystem->subsystem );
        }

        ZP_FORCEINLINE zp_bool_t operator==( const_ref_handle_type handle ) const
        {
            return m_subsystem == handle.m_subsystem;
        }

        ZP_FORCEINLINE zp_bool_t operator!=( const_ref_handle_type handle ) const
        {
            return m_subsystem != handle.m_subsystem;
        }

        ref_handle_type operator=( const_ref_handle_type handle )
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

        ref_handle_type operator=( move_handle_type handle ) noexcept
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

        explicit SubsystemHandle( SubsystemInstance* subsystem )
            : m_subsystem( subsystem )
        {
            if( m_subsystem )
            {
                m_subsystem->AddRef();
            }
        }

        SubsystemInstance* m_subsystem;

        friend class SubsystemManager;
    };

    class SubsystemManager
    {
    public:
        explicit SubsystemManager( MemoryLabel memoryLabel );

        ~SubsystemManager();

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

            SubsystemInstance* subsystem = FindSubsystem( typeHash );

            handle = zp_move( SubsystemHandle<T>( subsystem ) );
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

        [[nodiscard]] SubsystemInstance* FindSubsystem( zp_hash64_t id ) const;

        Vector<SubsystemInstance*> m_subsystems;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_SUBSYSTEM_H
