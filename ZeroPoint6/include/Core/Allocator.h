//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_ALLOCATOR_H
#define ZP_ALLOCATOR_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include <new>

#define ZP_NEW( l, t )                  new (zp::GetAllocator(static_cast<zp::MemoryLabel>(zp::MemoryLabels::l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(l)
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

        virtual void free( void* ptr ) = 0;
    };

    template<typename Storage, typename Policy>
    class MemoryAllocator final : public IMemoryAllocator
    {
        typedef MemoryAllocator<Storage, Policy> allocator_type;
    ZP_NONCOPYABLE( MemoryAllocator );

    private:
        typedef Storage storage_value;
        typedef const Storage& storage_const_reference;

        typedef Policy policy_value;
        typedef const Policy& policy_const_reference;

    public:
        MemoryAllocator( storage_const_reference storage, policy_const_reference policy );

        ~MemoryAllocator();

        void* allocate( zp_size_t size, zp_size_t alignment ) final;

        void free( void* ptr ) final;

    private:
        storage_value m_storage;
        policy_value m_policy;
    };

    void RegisterAllocator( MemoryLabel memoryLabel, IMemoryAllocator* memoryAllocator );

    IMemoryAllocator* GetAllocator( MemoryLabel memoryLabel );

    //
    //
    //

    struct MemoryLabelAllocator
    {
        MemoryLabelAllocator( MemoryLabel memoryLabel )
            : memoryLabel( memoryLabel )
            , alignment( kDefaultMemoryAlignment )
        {
        }

        MemoryLabelAllocator( MemoryLabel memoryLabel, zp_size_t alignment )
            : memoryLabel( memoryLabel )
            , alignment( alignment )
        {
        }

        void* allocate( zp_size_t size ) const
        {
            void* ptr = GetAllocator( memoryLabel )->allocate( size, alignment );
            return ptr;
        }

        void free( void* ptr ) const
        {
            GetAllocator( memoryLabel )->free( ptr );
        }

    private:
        const MemoryLabel memoryLabel;
        const zp_size_t alignment;
    };
}

//
// Impl
//

namespace zp
{
    template<typename Storage, typename Policy>
    MemoryAllocator<Storage, Policy>::MemoryAllocator(
        storage_const_reference storage,
        policy_const_reference policy )
        : m_storage( storage )
        , m_policy( policy )
    {
        void* memory = m_storage.memory();
        m_policy.set_memory( memory, m_storage.size());
    }

    template<typename Storage, typename Policy>
    MemoryAllocator<Storage, Policy>::~MemoryAllocator()
    {
        m_policy.clear_memory();

        m_storage.destroy();
    }

    template<typename Storage, typename Policy>
    void* MemoryAllocator<Storage, Policy>::allocate( const zp_size_t size, const zp_size_t alignment )
    {
        void* ptr = m_policy.allocate( size, alignment );
        return ptr;
    }

    template<typename Storage, typename Policy>
    void MemoryAllocator<Storage, Policy>::free( void* ptr )
    {
        m_policy.free( ptr );
    }
}

//
//
//

#include "Platform/Platform.h"

namespace zp
{
    class NullMemoryStorage
    {
    public:
        NullMemoryStorage( int )
        {
        }

        void* memory() const
        {
            return nullptr;
        }

        zp_size_t size() const
        {
            return 0;
        }

        void destroy() const
        {
        }
    };

    class SystemMemoryStorage
    {
    public:
        SystemMemoryStorage( void* baseAddress, zp_size_t size )
            : m_size( size )
            , m_baseAddress( baseAddress )
            , m_memory( nullptr )
        {
        }

        ~SystemMemoryStorage()
        {
            if( m_memory )
            {
                destroy();
            }
        }

        void* memory()
        {
            void* ptr = GetPlatform()->AllocateSystemMemory( m_baseAddress, m_size );
            m_memory = GetPlatform()->CommitMemoryPage( &ptr, m_size );
            return m_memory;
        }

        zp_size_t size() const
        {
            return m_size;
        }

        void destroy()
        {
            GetPlatform()->DecommitMemoryPage( m_memory, m_size );
            GetPlatform()->FreeSystemMemory( m_memory );
            m_memory = nullptr;
        }

    private:
        zp_size_t m_size;
        void* m_baseAddress;
        void* m_memory;
    };

}

#include <cstdlib>

namespace zp
{
    class SimpleAllocatorPolicy
    {
    public:
        SimpleAllocatorPolicy() = default;

        void set_memory( void* memory, zp_size_t size )
        {
        }

        void clear_memory()
        {
        }

        void* allocate( zp_size_t size, zp_size_t alignment )
        {
            void* ptr = ::_aligned_malloc( size, alignment );
            return ptr;
        }

        void free( void* ptr )
        {
            ::_aligned_free( ptr );
        }
    };

    class SystemPageAllocatorPolicy
    {
    public:
        void set_memory( void* memory, zp_size_t size )
        {
            m_memory = memory;
            m_size = size;
        }

        void clear_memory()
        {
            m_memory = nullptr;
            m_size = 0;
        }

        void* allocate( zp_size_t size, zp_size_t alignment )
        {
            return m_memory;
        }

        void free( const void* ptr )
        {
        }

    private:
        void* m_memory;
        zp_size_t m_size;
    };

}

#endif //ZP_ALLOCATOR_H
