//
// Created by phosg on 1/24/2022.
//

#ifndef ZP_ALLOCATOR_H
#define ZP_ALLOCATOR_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Memory.h"

#include <new>

namespace zp
{
    enum
    {
        kDefaultMemoryAlignment = 16,
        kMaxMemoryLabels = 16,
    };

    namespace MemoryLabels
    {
        constexpr MemoryLabel Default = 0;
        constexpr MemoryLabel String = 1;
        constexpr MemoryLabel Graphics = 2;
        constexpr MemoryLabel FileIO = 3;
        constexpr MemoryLabel Buffer = 4;
        constexpr MemoryLabel User = 5;
        constexpr MemoryLabel Data = 6;
        constexpr MemoryLabel Temp = 7;
        constexpr MemoryLabel ThreadSafe = 8;

        constexpr MemoryLabel Profiling = 9;
        constexpr MemoryLabel Debug = 10;

        constexpr MemoryLabel MemoryLabels_Count = 11;
    };

    ZP_STATIC_ASSERT( static_cast<MemoryLabel>( MemoryLabels::MemoryLabels_Count ) < kMaxMemoryLabels );
}

#define ZP_USE_MEMORY_PROFILER          ZP_USE_PROFILER

#define ZP_NEW( l, t )                  new (zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(static_cast<zp::MemoryLabel>(l))
#define ZP_NEW_ARGS( l, t, ... )        new (zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment)) t(static_cast<zp::MemoryLabel>(l),__VA_ARGS__)

#define ZP_DELETE( t, p )               do { const zp::MemoryLabel ZP_CONCAT(__memoryLabel_, __LINE__) = static_cast<t*>(p)->memoryLabel; static_cast<t*>(p)->~t(); zp::GetAllocator(ZP_CONCAT(__memoryLabel_, __LINE__))->free(p); p = nullptr; } while( false )
#define ZP_DELETE_LABEL( l, t, p )      do { p->~t(); zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->free(p); p = nullptr; } while( false )

#if ZP_USE_SAFE_DELETE
#define ZP_SAFE_DELETE( t, p )              do { if( p ) { ZP_DELETE( t, p ); } } while( false )
#define ZP_SAFE_DELETE_LABEL( l, t, p )     do { if( p ) { ZP_DELETE_LABEL( l, t, p ); } } while( false )
#else // !ZP_USE_SAFE_DELETE
#define ZP_SAFE_DELETE( t, p )          ZP_DELETE(t, p)
#define ZP_SAFE_DELETE_LABEL( l, t, p ) ZP_DELETE_LABEL(l, t, p)
#endif // ZP_USE_SAFE_DELETE

#define ZP_MALLOC( l, s )               zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate((s), zp::kDefaultMemoryAlignment)
#define ZP_ALIGNED_MALLOC( l, s, a )    zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate((s), (a))

#define ZP_MALLOC_T( l, t )                 static_cast<t*>(zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), zp::kDefaultMemoryAlignment))
#define ZP_ALIGNED_MALLOC_T( l, t, a )      static_cast<t*>(zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t), (a)))

#define ZP_MALLOC_T_ARRAY( l, t, c )                static_cast<t*>(zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t) * (c), zp::kDefaultMemoryAlignment))
#define ZP_ALIGNED_MALLOC_T_ARRAY( l, t, c, a )     static_cast<t*>(zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->allocate(sizeof(t) * (c), (a)))

#define ZP_REALLOC( l, p, s )                       zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->reallocate( (p), (s), zp::kDefaultMemoryAlignment)
#define ZP_ALIGNED_REALLOC( l, p, s, a )            zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->reallocate((p), (s), (a))

#define ZP_FREE( l, p )                 do { zp::GetAllocator(static_cast<zp::MemoryLabel>(l))->free(static_cast<void*>(p)); } while( false )

#if ZP_USE_SAFE_FREE
#define ZP_SAFE_FREE( l, p )            do { if( p ) { ZP_FREE( l, p ); } } while( false )
#else // !ZP_USE_SAFE_FREE
#define ZP_SAFE_FREE( l, p )            ZP_FREE( l, p )
#endif // ZP_USE_SAFE_FREE


namespace zp
{
    class ZP_DECLSPEC_NOVTABLE IMemoryAllocator
    {
    public:
        virtual void* allocate( zp_size_t size, zp_size_t alignment ) = 0;

        virtual void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment ) = 0;

        virtual void free( void* ptr ) = 0;
    };

    template<typename Storage, typename Policy, typename Locking, typename Profiler>
    class MemoryAllocator final : public IMemoryAllocator
    {
        using allocator_type = MemoryAllocator<Storage, Policy, Locking, Profiler>;
        ZP_NONCOPYABLE( MemoryAllocator );

    private:
        using storage_value = Storage;
        using storage_const_reference = const Storage&;

        using policy_value = Policy;
        using policy_reference = Policy&;
        using policy_const_reference = const Policy&;

        using lock_value = Locking;
        using lock_const_reference = const Locking&;

        using profiler_value = Profiler;
        using profiler_const_reference = const Profiler&;

    public:
        MemoryAllocator( storage_value storage = {}, policy_value policy = {}, lock_value locking = {}, profiler_value profiler = {} );

        ~MemoryAllocator() = default;

        void* allocate( zp_size_t size, zp_size_t alignment ) final;

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment ) final;

        void free( void* ptr ) final;

        policy_reference policy()
        {
            return m_policy;
        }

    private:
        storage_value m_storage;
        policy_value m_policy;
        lock_value m_lock;
        profiler_value m_profiler;
        MemoryLabel m_memoryLabel;
    };

    void RegisterAllocator( MemoryLabel memoryLabel, IMemoryAllocator* memoryAllocator );

    IMemoryAllocator* GetAllocator( MemoryLabel memoryLabel );

    //
    //
    //

    struct MemoryLabelAllocator
    {
        MemoryLabelAllocator()
            : alignment( kDefaultMemoryAlignment )
            , memoryLabel( 0 )
        {
        }

        explicit( false ) MemoryLabelAllocator( MemoryLabel memoryLabel )
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
        zp_size_t alignment;
        MemoryLabel memoryLabel;
    };
}

//
// Impl
//

namespace zp
{
    template<typename Storage, typename Policy, typename Locking, typename Profiler>
    MemoryAllocator<Storage, Policy, Locking, Profiler>::MemoryAllocator( storage_value storage, policy_value policy, lock_value locking, profiler_value profiler )
        : m_storage( zp_move( storage ) )
        , m_policy( zp_move( policy ) )
        , m_lock( zp_move( locking ) )
        , m_profiler( zp_move( profiler ) )
    {
        if( m_storage.is_fixed() )
        {
            zp_size_t requestedSize;
            void* mem = m_storage.request_memory( 0, requestedSize );
            m_policy.add_memory( mem, requestedSize );
        }
    }

    template<typename Storage, typename Policy, typename Locking, typename Profiler>
    void* MemoryAllocator<Storage, Policy, Locking, Profiler>::allocate( const zp_size_t size, const zp_size_t alignment )
    {
        m_lock.acquire();
        if( !m_storage.is_fixed() )
        {
            const zp_size_t allocSize = size + m_policy.overhead();
            const zp_size_t allocatedSize = m_policy.allocated();
            const zp_size_t totalSize = m_policy.total();

            if( ( allocatedSize + allocSize ) >= totalSize )
            {
                zp_size_t requestedSize;
                void* mem = m_storage.request_memory( allocSize, requestedSize );
                m_policy.add_memory( mem, requestedSize );
            }
        }

        void* ptr = m_policy.allocate( size, alignment );
        ZP_ASSERT( ptr );

        m_profiler.track_allocate( ptr, size, alignment, m_memoryLabel );

        m_lock.release();

        return ptr;
    }

    template<typename Storage, typename Policy, typename Locking, typename Profiler>
    void* MemoryAllocator<Storage, Policy, Locking, Profiler>::reallocate( void* oldPtr, const zp_size_t size, const zp_size_t alignment )
    {
        m_lock.acquire();
        if( !m_storage.is_fixed() )
        {
            const zp_size_t allocSize = size + m_policy.overhead();
            const zp_size_t allocatedSize = m_policy.allocated();
            const zp_size_t totalSize = m_policy.total();

            if( ( allocatedSize + allocSize ) >= totalSize )
            {
                zp_size_t requestedSize;
                void* mem = m_storage.request_memory( allocSize, requestedSize );
                m_policy.add_memory( mem, requestedSize );
            }
        }

        void* ptr = m_policy.reallocate( oldPtr, size, alignment );
        ZP_ASSERT( ptr );

        m_profiler.track_reallocate( oldPtr, ptr, size, alignment, m_memoryLabel );

        m_lock.release();

        return ptr;
    }

    template<typename Storage, typename Policy, typename Locking, typename Profiler>
    void MemoryAllocator<Storage, Policy, Locking, Profiler>::free( void* ptr )
    {
        m_lock.acquire();

        m_policy.free( ptr );

        m_profiler.track_free( ptr, m_memoryLabel );

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
        [[nodiscard]] ZP_FORCEINLINE void* request_memory( zp_size_t size, zp_size_t& requestedSize ) const
        {
            requestedSize = size;
            return nullptr;
        }

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t is_fixed() const
        {
            return true;
        }
    };

    struct NullAllocationPolicy
    {
        ZP_FORCEINLINE void add_memory( void* mem, zp_size_t size ) const
        {
        }

        [[nodiscard]] ZP_FORCEINLINE zp_size_t allocated() const
        {
            return 0;
        }

        [[nodiscard]] ZP_FORCEINLINE zp_size_t total() const
        {
            return 0;
        }

        [[nodiscard]] ZP_FORCEINLINE void* allocate( zp_size_t size, zp_size_t alignment )
        {
            return nullptr;
        }

        [[nodiscard]] ZP_FORCEINLINE void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment )
        {
            return nullptr;
        }

        ZP_FORCEINLINE void free( void* ptr )
        {
        }
    };

    struct NullMemoryLock
    {
        ZP_FORCEINLINE void acquire() const
        {
        };

        ZP_FORCEINLINE void release() const
        {
        };
    };

    struct NullMemoryProfiler
    {
        ZP_FORCEINLINE void track_allocate( void* ptr, zp_size_t size, zp_size_t alignment, MemoryLabel memoryLabel )
        {
        }

        ZP_FORCEINLINE void track_reallocate( void* oldPtr, void* ptr, zp_size_t size, zp_size_t alignment, MemoryLabel memoryLabel )
        {
        }

        ZP_FORCEINLINE void track_free( void* ptr, MemoryLabel memoryLabel )
        {
        }
    };
}
#pragma endregion

namespace zp
{
    struct TrackedMemoryProfiler
    {
        ZP_FORCEINLINE void track_allocate( void* ptr, zp_size_t size, zp_size_t alignment, MemoryLabel memoryLabel )
        {
        }

        ZP_FORCEINLINE void track_reallocate( void* oldPtr, void* ptr, zp_size_t size, zp_size_t alignment, MemoryLabel memoryLabel )
        {
        }

        ZP_FORCEINLINE void track_free( void* ptr, MemoryLabel memoryLabel )
        {
        }
    };
};

#pragma region CriticalSectionMemoryLock
namespace zp
{
    class CriticalSectionMemoryLock
    {
    public:
        void acquire();

        void release();

    private:
        //CriticalSection m_criticalSection;
    };
}
#pragma endregion

#pragma region System Page Memory Allocator
namespace zp
{
    class SystemPageMemoryStorage
    {
    public:
        SystemPageMemoryStorage( void* systemMemory, zp_size_t pageSize, zp_size_t totalSize );

        void* request_memory( zp_size_t size, zp_size_t& requestedSize );

        [[nodiscard]] zp_bool_t is_fixed() const;

    private:
        zp_size_t m_pageSize;
        zp_size_t m_totalSize;
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

        [[nodiscard]] zp_bool_t is_fixed() const;
    };

    class MallocAllocatorPolicy
    {
    public:
        void add_memory( void* mem, zp_size_t size );

        [[nodiscard]] zp_size_t allocated() const;

        [[nodiscard]] zp_size_t total() const;

        void* allocate( zp_size_t size, zp_size_t alignment );

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment );

        void free( void* ptr );

    protected:
        zp_size_t m_allocated {};
        zp_size_t m_size {};
    };
}
#pragma endregion

namespace zp
{
    struct LinearAllocatorPolicy
    {
        void add_memory( void* mem, zp_size_t size );

        [[nodiscard]] zp_size_t allocated() const;

        [[nodiscard]] zp_size_t total() const;

        [[nodiscard]] zp_size_t overhead() const;

        void* allocate( zp_size_t size, zp_size_t alignment );

        void* reallocate( void* ptr, zp_size_t size, zp_size_t alignment );

        void free( void* ptr );

        void reset();

    protected:
        void* m_ptr;
        zp_size_t m_allocated {};
        zp_size_t m_size {};
    };
}

#pragma region TLSF Memory Allocator
namespace zp
{
    class TlsfAllocatorPolicy
    {
    public:
        void add_memory( void* mem, zp_size_t size );

        [[nodiscard]] zp_size_t allocated() const;

        [[nodiscard]] zp_size_t total() const;

        [[nodiscard]] zp_size_t overhead() const;

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

namespace zp
{
    template<zp_size_t Size>
    class FixedMemoryStorage
    {
    public:
        void* request_memory( zp_size_t size, zp_size_t& requestedSize )
        {
            requestedSize = Size;
            return static_cast<void*>(m_memory);
        }

        [[nodiscard]] zp_bool_t is_fixed() const
        {
            return true;
        }

    private:
        zp_uint8_t m_memory[ Size ];
    };
}

namespace zp
{
    class FixedAllocatedMemoryStorage
    {
    public:
        FixedAllocatedMemoryStorage( void* memory, zp_size_t size )
            : m_memory( memory )
            , m_size( size )
        {
        }

        void* request_memory( zp_size_t size, zp_size_t& requestedSize )
        {
            ZP_ASSERT( size <= m_size );
            requestedSize = m_size;
            return static_cast<void*>(m_memory);
        }

        [[nodiscard]] zp_bool_t is_fixed() const
        {
            return true;
        }

    private:
        void* m_memory;
        zp_size_t m_size;
    };

    class FixedArenaMemoryStorage
    {
    public:
        FixedArenaMemoryStorage( MemoryLabel memoryLabel, zp_size_t size )
            : m_memory( ZP_MALLOC( memoryLabel, size ) )
            , m_size( size )
            , m_memoryLabel( memoryLabel )
        {
        }

        ~FixedArenaMemoryStorage()
        {
            ZP_SAFE_FREE( m_memoryLabel, m_memory );
        }

        void* request_memory( zp_size_t size, zp_size_t& requestedSize )
        {
            ZP_ASSERT( size <= m_size );
            requestedSize = m_size;
            return m_memory;
        }

        [[nodiscard]] zp_bool_t is_fixed() const
        {
            return true;
        }

    private:
        void* m_memory;
        zp_size_t m_size;
        MemoryLabel m_memoryLabel;
    };

    using ManualArenaAllocator = MemoryAllocator<FixedAllocatedMemoryStorage, LinearAllocatorPolicy, NullMemoryLock, NullMemoryProfiler>;
    using ArenaAllocator = MemoryAllocator<FixedArenaMemoryStorage, LinearAllocatorPolicy, NullMemoryLock, NullMemoryProfiler>;
}
#if 1
namespace zp
{
    struct AllocMemory
    {
        void* ptr;
        zp_size_t size;
        MemoryLabel memoryLabel;

        AllocMemory()
            : ptr( nullptr )
            , size( 0 )
            , memoryLabel( 0 )
        {
        }

        AllocMemory( MemoryLabel memoryLabel, zp_size_t size )
            : ptr( size ? ZP_MALLOC( memoryLabel, size ) : nullptr )
            , size( size )
            , memoryLabel( memoryLabel )
        {
        }

        AllocMemory( MemoryLabel memoryLabel, Memory memory )
            : ptr( memory.size ? ZP_MALLOC( memoryLabel, memory.size ) : nullptr )
            , size( memory.size )
            , memoryLabel( memoryLabel )
        {
            zp_memcpy( ptr, size, memory.ptr, memory.size );
        }

        AllocMemory( const AllocMemory& other )
            : ptr( other.size ? ZP_MALLOC( other.memoryLabel, other.size ) : nullptr )
            , size( other.size )
            , memoryLabel( other.memoryLabel )
        {
            zp_memcpy( ptr, size, other.ptr, other.size );
        }

        AllocMemory( AllocMemory&& other ) noexcept
            : ptr( other.ptr )
            , size( other.size )
            , memoryLabel( other.memoryLabel )
        {
            other.ptr = nullptr;
            other.size = 0;
        }

        ~AllocMemory()
        {
            ZP_SAFE_FREE( memoryLabel, ptr );
            size = 0;
        }

        AllocMemory& operator=( const AllocMemory& other )
        {
            ZP_SAFE_FREE( memoryLabel, ptr );

            ptr = other.ptr;
            size = other.size;
            memoryLabel = other.memoryLabel;

            return *this;
        }

        AllocMemory& operator=( AllocMemory&& other )
        {
            ZP_SAFE_FREE( memoryLabel, ptr );

            ptr = other.ptr;
            size = other.size;
            memoryLabel = other.memoryLabel;

            other.ptr = nullptr;
            other.size = 0;

            return *this;
        }

        template<typename T>
        ZP_FORCEINLINE T* as()
        {
            ZP_ASSERT( sizeof( T ) <= size );
            return static_cast<T*>(ptr);
        }

        template<typename T>
        ZP_FORCEINLINE const T* as() const
        {
            ZP_ASSERT( sizeof( T ) <= size );
            return static_cast<const T*>(ptr);
        }

        [[nodiscard]] ZP_FORCEINLINE Memory memory() const
        {
            return { .ptr = ptr, .size = size };
        }

        [[nodiscard]] ZP_FORCEINLINE Memory slice( zp_size_t offset, zp_size_t sz ) const
        {
            return {
                .ptr = ZP_OFFSET_PTR( ptr, offset ),
                .size = sz
            };
        }
    };
}
#endif

#endif //ZP_ALLOCATOR_H
