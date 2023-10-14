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
        IMemoryAllocator* s_memoryAllocators[static_cast<MemoryLabel>(kMaxMemoryLabels)];
    }

    void RegisterAllocator( const MemoryLabel memoryLabel, IMemoryAllocator* memoryAllocator )
    {
        ZP_ASSERT( memoryLabel < static_cast<MemoryLabel>(kMaxMemoryLabels) );
        s_memoryAllocators[ memoryLabel ] = memoryAllocator;
    }

    IMemoryAllocator* GetAllocator( const MemoryLabel memoryLabel )
    {
        ZP_ASSERT( memoryLabel < static_cast<MemoryLabel>(kMaxMemoryLabels) );
        return s_memoryAllocators[ memoryLabel ];
    }
}

namespace zp
{
    SystemPageMemoryStorage::SystemPageMemoryStorage( void* systemMemory, zp_size_t pageSize )
        : m_pageSize( pageSize )
        , m_systemMemory( systemMemory )
        , m_memory( systemMemory )
    {
    }

    void* SystemPageMemoryStorage::request_memory( zp_size_t size, zp_size_t& requestedSize )
    {
        requestedSize = 0;
        while( requestedSize < size )
        {
            requestedSize += m_pageSize;
        }

        void* mem = GetPlatform()->CommitMemoryPage( &m_memory, requestedSize );
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
        void* mem = _aligned_malloc( requestedSize, kDefaultMemoryAlignment );
        return mem;
    }

    zp_bool_t MallocMemoryStorage::is_fixed() const
    {
        return false;
    }

    //
    //
    //

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
        void* mem = _aligned_malloc( size, alignment );
        return mem;
    }

    void* MallocAllocatorPolicy::reallocate( void* ptr, zp_size_t size, zp_size_t alignment )
    {
        void* mem = _aligned_realloc( ptr, size, alignment );
        return mem;
    }

    void MallocAllocatorPolicy::free( void* ptr )
    {
        _aligned_free( ptr );
    }
}

namespace zp
{
    void TlsfAllocatorPolicy::add_memory( void* mem, zp_size_t size )
    {
        if( m_tlsf )
        {
            tlsf_add_pool( m_tlsf, mem, size );
            m_size += size - tlsf_pool_overhead();
        }
        else
        {
            m_tlsf = tlsf_create_with_pool( mem, size );
            m_size += size - ( tlsf_size() + tlsf_pool_overhead() );
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
        m_criticalSection.enter();
    }

    void CriticalSectionMemoryLock::release()
    {
        m_criticalSection.leave();
    }
}
