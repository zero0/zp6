//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_ALLOCATOR_H
#define ZP_ALLOCATOR_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include <new>

#define ZP_NEW( l, t )                  new (zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(zp::MemoryLabels::l)
#define ZP_NEW_ARGS( l, t, ... )        new (zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l),__VA_ARGS__)

#define ZP_NEW_( l, t )                 new (zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(l)
#define ZP_NEW_ARGS_( l, t, ... )       new (zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(l,__VA_ARGS__)

#define ZP_DELETE( t, p )               do { const zp::MemoryLabel ZP_CONCAT(__memoryLabel_, __LINE__) = p->memoryLabel; p->~t(); zp::GetAllocator(ZP_CONCAT(__memoryLabel_, __LINE__))->free(p); p = nullptr; } while( false )
#define ZP_DELETE_( l, t, p )           do { p->~t(); zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->free(p); p = nullptr; } while( false )
#define ZP_DELETE_LABEL( l, t, p )      do { p->~t(); zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->free(p); p = nullptr; } while( false )

#if ZP_USE_SAFE_DELETE
#define ZP_SAFE_DELETE( t, p )            do { if( p ) { const zp::MemoryLabel ZP_CONCAT(__memoryLabel_, __LINE__) = p->memoryLabel; p->~t(); zp::GetAllocator(ZP_CONCAT(__memoryLabel_, __LINE__))->free(p); p = nullptr; } } while( false )
#define ZP_SAFE_DELETE_( l, t, p )        do { if( p ) { p->~t(); zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->free(p); p = nullptr; } } while( false )
#define ZP_SAFE_DELETE_LABEL( l, t, p )   do { if( p ) { p->~t(); zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->free(p); p = nullptr; } } while( false )
#else // !ZP_USE_SAFE_DELETE
#define ZP_SAFE_DELETE( t, p )          ZP_DELETE(t, p)
#define ZP_SAFE_DELETE_( l, t, p )      ZP_DELETE_(l, t, p)
#define ZP_SAFE_DELETE_LABEL( l, t, p ) ZP_DELETE_LABEL(l, t, p)
#endif

#define ZP_MALLOC( l, s )               zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->allocate((s), zp::kDefaultMemoryAlignment)
#define ZP_ALIGNED_MALLOC( l, s, a )    zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->allocate((s), (a))
#define ZP_FREE( l, p )                 zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->free((p))

#define ZP_MALLOC_( l, s )              zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate((s), zp::kDefaultMemoryAlignment)
#define ZP_MALLOC_T( l, t )             static_cast<t*>(zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment))
#define ZP_MALLOC_T_ARRAY( l, t, c )    static_cast<t*>(zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t) * (c), zp::kDefaultMemoryAlignment))
#define ZP_ALIGNED_MALLOC_( l, s, a )   zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate((s), (a))
#define ZP_FREE_( l, p )                zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->free((p))

#define ZP_SAFE_FREE_LABEL( l, p )      do { if( p ) { zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->free((p)); } } while( false )

#define KB                              * zp_size_t( 1024 )
#define MB                              * zp_size_t( 1024 * 1024 )

namespace zp
{
    typedef zp_uint8_t MemoryLabel;

    enum
    {
        kDefaultMemoryAlignment = 16,
        kMaxMemoryLabels = 16
    };

    class ZP_DECLSPEC_NOVTABLE IMemoryAllocator
    {
    public:
        virtual void* allocate( zp_size_t size, zp_size_t alignment ) = 0;

        virtual void* reallocate( void* ptr, zp_size_t size, zp_size_t alignent ) = 0;

        virtual void free( void* ptr ) = 0;
    };

    template<typename Storage, typename Policy, typename Locking>
    class MemoryAllocator final : public IMemoryAllocator
    {
        typedef MemoryAllocator<Storage, Policy, Locking> allocator_type;
    ZP_NONCOPYABLE( MemoryAllocator );

    private:
        typedef Storage storage_value;
        typedef const Storage& storage_const_reference;

        typedef Policy policy_value;
        typedef const Policy& policy_const_reference;

        typedef Locking lock_value;
        typedef const Locking& lock_const_reference;

    public:
        MemoryAllocator( storage_const_reference storage, policy_const_reference policy, lock_const_reference locking );

        ~MemoryAllocator() = default;

        void* allocate( zp_size_t size, zp_size_t alignment ) final;

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment ) final;

        void free( void* ptr ) final;

    private:
        storage_value m_storage;
        policy_value m_policy;
        lock_value m_lock;
    };

    void RegisterAllocator( MemoryLabel memoryLabel, IMemoryAllocator* memoryAllocator );

    IMemoryAllocator* GetAllocator( MemoryLabel memoryLabel );

    //
    //
    //

    struct MemoryLabelAllocator
    {
        MemoryLabelAllocator( MemoryLabel memoryLabel )
            : alignment( kDefaultMemoryAlignment )
            , memoryLabel( memoryLabel )
        {
        }

        MemoryLabelAllocator( MemoryLabel memoryLabel, zp_size_t alignment )
            : alignment( alignment )
            , memoryLabel( memoryLabel )
        {
        }

        [[nodiscard]] void* allocate( zp_size_t size ) const
        {
            void* ptr = GetAllocator( memoryLabel )->allocate( size, alignment );
            return ptr;
        }

        void free( void* ptr ) const
        {
            GetAllocator( memoryLabel )->free( ptr );
        }

    private:
        const zp_size_t alignment;
        const MemoryLabel memoryLabel;
    };
}

//
// Impl
//

namespace zp
{
    template<typename Storage, typename Policy, typename Locking>
    MemoryAllocator<Storage, Policy, Locking>::MemoryAllocator( storage_const_reference storage, policy_const_reference policy, lock_const_reference locking )
        : m_storage( storage )
        , m_policy( policy )
        , m_lock( locking )
    {
        if( m_storage.is_fixed() )
        {
            zp_size_t requestedSize;
            void* mem = m_storage.request_memory( 0, requestedSize );
            m_policy.add_memory( mem, requestedSize );
        }
    }

    template<typename Storage, typename Policy, typename Locking>
    void* MemoryAllocator<Storage, Policy, Locking>::allocate( const zp_size_t size, const zp_size_t alignment )
    {
        m_lock.acquire();
        if( !m_storage.is_fixed() )
        {
            const zp_size_t allocatedSize = m_policy.allocated();
            const zp_size_t totalSize = m_policy.total();

            if( ( allocatedSize + size ) >= totalSize )
            {
                zp_size_t requestedSize;
                void* mem = m_storage.request_memory( size, requestedSize );
                m_policy.add_memory( mem, requestedSize );
            }
        }

        void* ptr = m_policy.allocate( size, alignment );
        ZP_ASSERT( ptr );
        m_lock.release();

        return ptr;
    }

    template<typename Storage, typename Policy, typename Locking>
    void* MemoryAllocator<Storage, Policy, Locking>::reallocate( void* oldPtr, const zp_size_t size, const zp_size_t alignment )
    {
        m_lock.acquire();
        if( !m_storage.is_fixed() )
        {
            const zp_size_t allocatedSize = m_policy.allocated();
            const zp_size_t totalSize = m_policy.total();

            if( ( allocatedSize + size ) >= totalSize )
            {
                zp_size_t requestedSize;
                void* mem = m_storage.request_memory( size, requestedSize );
                m_policy.add_memory( mem, requestedSize );
            }
        }

        void* ptr = m_policy.reallocate( oldPtr, size, alignment );
        ZP_ASSERT( ptr );
        m_lock.release();

        return ptr;
    }

    template<typename Storage, typename Policy, typename Locking>
    void MemoryAllocator<Storage, Policy, Locking>::free( void* ptr )
    {
        m_lock.acquire();

        m_policy.free( ptr );

        m_lock.release();
    }
}

//
//
//

#pragma region Null Memory Impl
namespace zp
{
    struct NullMemoryStorage
    {
        void* request_memory( zp_size_t size, zp_size_t& requestedSize ) const
        {
            requestedSize = size;
            return nullptr;
        }

        zp_bool_t is_fixed() const
        {
            return true;
        }
    };

    struct NullAllocationPolicy
    {
        void add_memory( void* mem, zp_size_t size ) const
        {
        }

        zp_size_t allocated() const
        {
            return 0;
        }

        zp_size_t total() const
        {
            return 0;
        }

        void* allocate( zp_size_t size, zp_size_t alignment )
        {
            return nullptr;
        }

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment )
        {
            return nullptr;
        }

        void free( void* ptr )
        {
        }
    };

    struct NullMemoryLock
    {
        void acquire() const
        {
        };

        void release() const
        {
        };
    };

}
#pragma endregion

#pragma region System Page Memory Allocator
namespace zp
{
    class SystemPageMemoryStorage
    {
    public:
        SystemPageMemoryStorage( void* systemMemory, zp_size_t pageSize );

        void* request_memory( zp_size_t size, zp_size_t& requestedSize );

        zp_bool_t is_fixed() const;

    private:
        zp_size_t m_pageSize;
        void* m_systemMemory;
        void* m_memory;
    };
}
#pragma endregion

#pragma region Malloc Memory Allocator
namespace zp
{
    struct MallocMemoryStorage
    {
        void* request_memory( zp_size_t size, zp_size_t& requestedSize );

        zp_bool_t is_fixed() const;
    };

    class MallocAllocatorPolicy
    {
    public:
        void add_memory( void* mem, zp_size_t size );

        zp_size_t allocated() const;

        zp_size_t total() const;

        void* allocate( zp_size_t size, zp_size_t alignment );

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment );

        void free( void* ptr );

    protected:
        zp_size_t m_allocated;
        zp_size_t m_size;
    };
}
#pragma endregion

#pragma region TLSF Memory Allocator
namespace zp
{
    class TlsfAllocatorPolicy
    {
    public:
        void add_memory( void* mem, zp_size_t size );

        zp_size_t allocated() const;

        zp_size_t total() const;

        void* allocate( zp_size_t size, zp_size_t alignment );

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment );

        void free( void* ptr );

    private:
        zp_handle_t m_tlsf;
        zp_size_t m_allocated;
        zp_size_t m_size;
    };
}
#pragma endregion

#endif //ZP_ALLOCATOR_H
