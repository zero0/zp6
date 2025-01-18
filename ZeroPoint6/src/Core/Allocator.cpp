//
// Created by phosg on 1/24/2022.
//

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

#include "Platform/Platform.h"

#include "tlsf/tlsf.h"

#include <cstdlib>

namespace zp
{
    namespace
    {
        IMemoryAllocator* s_memoryAllocators[kMaxMemoryLabels];
    }

    void RegisterAllocator( const MemoryLabel memoryLabel, IMemoryAllocator* memoryAllocator )
    {
        ZP_ASSERT( memoryLabel < kMaxMemoryLabels );
        s_memoryAllocators[ memoryLabel ] = memoryAllocator;
    }

    IMemoryAllocator* GetAllocator( const MemoryLabel memoryLabel )
    {
        ZP_ASSERT( memoryLabel < kMaxMemoryLabels );
        return s_memoryAllocators[ memoryLabel ];
    }
}

namespace zp
{
    SystemPageMemoryStorage::SystemPageMemoryStorage( void* systemMemory, zp_size_t pageSize, zp_size_t totalSize )
        : m_pageSize( pageSize )
        , m_totalSize( totalSize )
        , m_systemMemory( systemMemory )
        , m_memory( systemMemory )
    {
    }

    void* SystemPageMemoryStorage::request_memory( zp_size_t size, zp_size_t& requestedSize )
    {
        requestedSize = 0;

        if( m_pageSize == 0 )
        {
            requestedSize = m_totalSize;
        }
        else
        {
            while( requestedSize < size )
            {
                requestedSize += m_pageSize;
            }
        }

        void* mem = Platform::CommitMemoryPage( &m_memory, requestedSize );
        return mem;
    }

    zp_bool_t SystemPageMemoryStorage::is_fixed() const
    {
        return false;
    }
}

namespace zp
{
    void* MallocMemoryStorage::request_memory( zp_size_t size, zp_size_t& requestedSize )
    {
        requestedSize = size;
        void* mem = ::_aligned_malloc( requestedSize, kDefaultMemoryAlignment );
        return mem;
    }

    zp_bool_t MallocMemoryStorage::is_fixed() const
    {
        return false;
    }

    //
    //
    //

    namespace
    {
        struct MallocHeader
        {
            zp_size_t allocatedSize;
        };
    }

    void MallocAllocatorPolicy::add_memory( void* mem, zp_size_t size )
    {
        m_size += size;
    }

    zp_size_t MallocAllocatorPolicy::allocated() const
    {
        return m_allocated;
    }

    zp_size_t MallocAllocatorPolicy::total() const
    {
        return m_size;
    }

    void* MallocAllocatorPolicy::allocate( zp_size_t size, zp_size_t alignment )
    {
#if ZP_USE_MEMORY_PROFILER
        const zp_size_t allocatedSize = size + sizeof( MallocHeader );
#else
        const zp_size_t allocatedSize = size;
#endif

        void* mem = ::_aligned_malloc( allocatedSize, alignment );
        ZP_ASSERT( mem );

#if ZP_USE_MEMORY_PROFILER
        if( mem )
        {
            m_allocated += allocatedSize;

            MallocHeader* header = reinterpret_cast<MallocHeader*>( mem );
            header->allocatedSize = allocatedSize;

            mem = header + 1;
        }
#endif

        return mem;
    }

    void* MallocAllocatorPolicy::reallocate( void* ptr, zp_size_t size, zp_size_t alignment )
    {
        void* mem;

#if ZP_USE_MEMORY_PROFILER
        if( ptr )
        {
            MallocHeader* header = reinterpret_cast<MallocHeader*>( ptr ) - 1;
            m_allocated -= header->allocatedSize;

            ptr = header;
        }

        const zp_size_t allocatedSize = size + sizeof( MallocHeader );
        mem = ::_aligned_realloc( ptr, allocatedSize, alignment );

        if( mem )
        {
            m_allocated += allocatedSize;

            MallocHeader* header = reinterpret_cast<MallocHeader*>( mem );
            header->allocatedSize = allocatedSize;

            mem = header + 1;
        }
#else
            mem = _aligned_realloc( ptr, size, alignment );
#endif
        ZP_ASSERT( mem );
        return mem;
    }

    void MallocAllocatorPolicy::free( void* ptr )
    {
#if ZP_USE_MEMORY_PROFILER
        if( ptr )
        {
            MallocHeader* header = reinterpret_cast<MallocHeader*>( ptr ) - 1;
            m_allocated -= header->allocatedSize;

            ptr = header;
        }
#endif

        _aligned_free( ptr );
    }
}

namespace zp
{
    void LinearAllocatorPolicy::add_memory( void* mem, zp_size_t size )
    {
        m_ptr = mem;
        m_allocated = 0;
        m_size = size;
    }

    zp_size_t LinearAllocatorPolicy::allocated() const
    {
        return m_allocated;
    }

    zp_size_t LinearAllocatorPolicy::total() const
    {
        return m_size;
    }

    void* LinearAllocatorPolicy::allocate( zp_size_t size, zp_size_t alignment )
    {
        zp_uint8_t* mem = static_cast<zp_uint8_t*>( m_ptr ) + m_allocated;
        m_allocated += size;
        return mem;
    }

    void* LinearAllocatorPolicy::reallocate( void* ptr, zp_size_t size, zp_size_t alignment )
    {
        zp_uint8_t* mem = static_cast<zp_uint8_t*>( m_ptr ) + m_allocated;
        m_allocated += size;
        return mem;
    }

    void LinearAllocatorPolicy::free( void* ptr )
    {
        // no-op
    }

    void LinearAllocatorPolicy::reset()
    {
        m_allocated = 0;
    }
}

namespace zp
{
    void TlsfAllocatorPolicy::add_memory( void* mem, zp_size_t size )
    {
        if( m_tlsf == nullptr )
        {
            m_tlsf = tlsf_create_with_pool( mem, size );
            m_size += size - ( tlsf_size() + tlsf_pool_overhead() );
        }
        else
        {
            tlsf_add_pool( m_tlsf, mem, size );
            m_size += size - tlsf_pool_overhead();
        }
    }

    zp_size_t TlsfAllocatorPolicy::allocated() const
    {
        return m_allocated;
    }

    zp_size_t TlsfAllocatorPolicy::total() const
    {
        return m_size;
    }

    zp_size_t TlsfAllocatorPolicy::overhead() const
    {
        return m_tlsf == nullptr ? tlsf_size() + tlsf_pool_overhead() : tlsf_pool_overhead();
    }

    void* TlsfAllocatorPolicy::allocate( zp_size_t size, zp_size_t alignment )
    {
        void* mem = tlsf_memalign( m_tlsf, alignment, size );
        m_allocated += tlsf_block_size( mem );
        return mem;
    }

    void* TlsfAllocatorPolicy::reallocate( void* ptr, zp_size_t size, zp_size_t alignment )
    {
        m_allocated -= tlsf_block_size( ptr );
        void* mem = tlsf_realloc( m_tlsf, ptr, size );
        m_allocated += tlsf_block_size( mem );
        return mem;
    }

    void TlsfAllocatorPolicy::free( void* ptr )
    {
        m_allocated -= tlsf_block_size( ptr );
        tlsf_free( m_tlsf, ptr );
    }
}

namespace zp
{
    void CriticalSectionMemoryLock::acquire()
    {
        //Platform::EnterCriticalSection( m_criticalSection );
    }

    void CriticalSectionMemoryLock::release()
    {
        //Platform::LeaveCriticalSection( m_criticalSection );
    }
}
