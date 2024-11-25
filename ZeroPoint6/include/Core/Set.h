//
// Created by phosg on 2/3/2022.
//

#ifndef ZP_SET_H
#define ZP_SET_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Common.h"
#include "Core/Allocator.h"
#include "Core/Hash.h"

namespace zp
{
    template<typename T, typename H = zp_hash64_t, typename Comparer = EqualityComparer <T, H>, typename Allocator = MemoryLabelAllocator>
    class Set
    {
    public:
        static const zp_size_t npos = -1;
        static constexpr H nhash = zp_fnv_1a<H>( nullptr, 0 );

        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef H hash_value;
        typedef const H const_hash_value;

        class SetIterator;

        class SetConstIterator;

        typedef SetIterator iterator;
        typedef SetConstIterator const_iterator;

        typedef Comparer comparer_value;
        typedef const Comparer& comparer_const_reference;

        typedef Allocator allocator_value;
        typedef const allocator_value& allocator_const_reference;

        typedef Set<T, H, Comparer, Allocator> self_value;
        typedef const Set<T, H, Comparer, Allocator>& const_self_reference;
        typedef const Set<T, H, Comparer, Allocator>* const_self_pointer;
        typedef Set<T, H, Comparer, Allocator>* self_pointer;

        Set();

        explicit Set( zp_size_t capacity );

        explicit Set( comparer_const_reference cmp );

        explicit Set( allocator_const_reference allocator );

        Set( zp_size_t capacity, comparer_const_reference cmp );

        Set( zp_size_t capacity, allocator_const_reference allocator );

        Set( zp_size_t capacity, comparer_const_reference cmp, allocator_const_reference allocator );

        Set( comparer_const_reference cmp, allocator_const_reference allocator );

        ~Set();

        zp_bool_t add( const_reference value );

        zp_bool_t remove( const_reference value );

        void unionWith( const_pointer ptr, zp_size_t count );

        void unionWith( const_iterator begin, const_iterator end );

        void exceptWith( const_pointer ptr, zp_size_t count );

        void exceptWith( const_iterator begin, const_iterator end );

        zp_bool_t contains( const_reference value ) const;

        [[nodiscard]] zp_size_t size() const;

        [[nodiscard]] zp_bool_t isEmpty() const;

        void clear();

        void reserve( zp_size_t size );

        void destroy();

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;

    private:
        void ensureCapacity( zp_size_t capacity, zp_bool_t forceRehash );

        zp_size_t findIndex( const_reference value ) const;

        struct SetEntry
        {
            hash_value hash;
            zp_size_t next;
            value_type value;
        };

        SetEntry* m_entries;
        zp_size_t* m_buckets;

        zp_size_t m_capacity;
        zp_size_t m_count;
        zp_size_t m_freeCount;
        zp_size_t m_freeList;

        comparer_value m_cmp;
        allocator_value m_allocator;


    public:
        class SetIterator
        {
        private:
            SetIterator();

            SetIterator( self_pointer set, zp_size_t index );

        public:
            ~SetIterator() = default;

            void operator++();

            void operator++( int );

            pointer operator->();

            const_pointer operator->() const;

            const_reference operator*() const;

            zp_bool_t operator==( const SetIterator& other ) const;

            zp_bool_t operator!=( const SetIterator& other ) const;

        private:
            self_pointer m_set;
            zp_size_t m_current;

            friend class Set;
        };

        class SetConstIterator
        {
        private:
            SetConstIterator();

            SetConstIterator( const_self_pointer set, zp_size_t index );

        public:
            ~SetConstIterator() = default;

            void operator++();

            void operator++( int );

            const_pointer operator->() const;

            const_reference operator*() const;

            zp_bool_t operator==( const SetConstIterator& other ) const;

            zp_bool_t operator!=( const SetConstIterator& other ) const;

        private:
            const_self_pointer m_set;
            zp_size_t m_current;

            friend class Set;
        };
    };
}

//
//
//

namespace zp
{
    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set()
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( comparer_value() )
        , m_allocator( allocator_value() )
    {
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( zp_size_t capacity )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( comparer_value() )
        , m_allocator( allocator_value() )
    {
        ensureCapacity( capacity, false );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( comparer_const_reference cmp )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( cmp )
        , m_allocator( allocator_value() )
    {
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( allocator_const_reference allocator )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( comparer_value() )
        , m_allocator( allocator )
    {
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( zp_size_t capacity, comparer_const_reference cmp )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( cmp )
        , m_allocator( allocator_value() )
    {
        ensureCapacity( capacity, false );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( zp_size_t capacity, allocator_const_reference allocator )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( comparer_value() )
        , m_allocator( allocator )
    {
        ensureCapacity( capacity, false );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( zp_size_t capacity, comparer_const_reference cmp, allocator_const_reference allocator )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( cmp )
        , m_allocator( allocator )
    {
        ensureCapacity( capacity, false );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::Set( comparer_const_reference cmp, allocator_const_reference allocator )
        : m_entries( nullptr )
        , m_buckets( nullptr )
        , m_capacity( 0 )
        , m_count( 0 )
        , m_freeCount( 0 )
        , m_freeList( 0 )
        , m_cmp( cmp )
        , m_allocator( allocator )
    {
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::~Set()
    {
        destroy();
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::add( const_reference value )
    {
        if( m_buckets == nullptr )
        {
            ensureCapacity( 4, false );
        }

        const_hash_value hash = m_cmp.hash( value );
        zp_size_t index = hash % m_capacity;
        zp_size_t numSteps = 0;

        for( zp_size_t b = m_buckets[ index ]; b != npos; b = m_entries[ b ].next )
        {
            if( m_entries[ b ].hash == hash && m_cmp.equals( m_entries[ b ].value, value ) )
            {
                return false;
            }
            ++numSteps;
        }

        zp_size_t newIndex;
        if( m_freeCount > 0 )
        {
            newIndex = m_freeList;
            m_freeList = m_entries[ newIndex ].next;
            --m_freeCount;
        }
        else
        {
            if( m_count == m_capacity )
            {
                ensureCapacity( m_capacity * 2, false );
                index = hash % m_capacity;
            }
            newIndex = m_count;
            ++m_count;
        }

        m_entries[ newIndex ].hash = hash;
        m_entries[ newIndex ].next = m_buckets[ index ];
        m_entries[ newIndex ].value = zp_move( value );

        m_buckets[ index ] = newIndex;

        if( numSteps >= m_capacity )
        {
            ensureCapacity( m_capacity + 1, true );
        }

        return true;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::remove( const_reference value )
    {
        zp_bool_t removed = false;

        if( m_buckets != nullptr )
        {
            const_hash_value hash = m_cmp.hash( value );
            const zp_size_t index = hash % m_capacity;
            zp_size_t removeIndex = npos;

            for( zp_size_t b = m_buckets[ index ]; b != npos; b = m_entries[ b ].next )
            {
                if( m_entries[ b ].hash == hash && m_cmp.equals( m_entries[ b ].value, value ) )
                {
                    if( removeIndex == npos )
                    {
                        m_buckets[ index ] = m_entries[ b ].next;
                    }
                    else
                    {
                        m_entries[ removeIndex ].next = m_entries[ b ].next;
                    }

                    m_entries[ b ].hash = npos;
                    m_entries[ b ].next = m_freeList;
                    ( &m_entries[ b ].value )->~value_type();

                    m_freeList = b;
                    ++m_freeCount;

                    removed = true;
                    break;
                }

                removeIndex = b;
            }
        }

        return removed;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::unionWith( const_pointer ptr, zp_size_t count )
    {
        for( zp_size_t i = 0; i < count; ++i )
        {
            add( *( ptr + i ) );
        }
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::unionWith( const_iterator begin, const_iterator end )
    {
        for( ; begin != end; ++begin )
        {
            add( *begin );
        }
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::exceptWith( const_pointer ptr, zp_size_t count )
    {
        for( zp_size_t i = 0; i < count; ++i )
        {
            remove( *( ptr + i ) );
        }
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::exceptWith( const_iterator begin, const_iterator end )
    {
        for( ; begin != end; ++begin )
        {
            remove( *begin );
        }
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::contains( const_reference value ) const
    {
        const zp_size_t index = findIndex( value );
        return index != npos;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_size_t Set<T, H, Comparer, Allocator>::size() const
    {
        return m_count - m_freeCount;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::isEmpty() const
    {
        return ( m_count - m_freeCount ) == 0;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::clear()
    {
        if( m_count > 0 )
        {
            for( zp_size_t i = 0; i < m_capacity; ++i )
            {
                m_buckets[ i ] = npos;
            }

            for( zp_size_t i = 0; i < m_count; ++i )
            {
                ( m_entries + i )->~SetEntry();
            }

            m_freeList = npos;
            m_freeCount = 0;
            m_count = 0;
        }
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::reserve( zp_size_t size )
    {
        ensureCapacity( size, false );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::destroy()
    {
        clear();

        m_allocator.free( m_buckets );
        m_buckets = nullptr;

        m_allocator.free( m_entries );
        m_entries = nullptr;

        m_capacity = 0;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::iterator Set<T, H, Comparer, Allocator>::begin()
    {
        zp_size_t index = 0;
        while( index < m_count && m_entries[ index ].hash == nhash )
        {
            ++index;
        }

        return iterator( this, index );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::iterator Set<T, H, Comparer, Allocator>::end()
    {
        return iterator( this, m_count );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::const_iterator Set<T, H, Comparer, Allocator>::begin() const
    {
        zp_size_t index = 0;
        while( index < m_count && m_entries[ index ].hash == nhash )
        {
            ++index;
        }

        return const_iterator( this, index );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::const_iterator Set<T, H, Comparer, Allocator>::end() const
    {
        return const_iterator( this, m_count );
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::ensureCapacity( zp_size_t capacity, zp_bool_t forceRehash )
    {
        if( capacity > m_capacity )
        {
            zp_size_t* buckets = static_cast<zp_size_t*>( m_allocator.allocate( sizeof( zp_size_t ) * capacity ));
            SetEntry* entries = static_cast<SetEntry*>( m_allocator.allocate( sizeof( SetEntry ) * capacity ));
            zp_zero_memory_array( entries, capacity );

            for( zp_size_t i = 0; i < capacity; ++i )
            {
                buckets[ i ] = npos;
            }

            if( m_entries != nullptr )
            {
                for( zp_size_t i = 0; i < m_count; ++i )
                {
                    entries[ i ] = zp_move( m_entries[ i ] );
                }

                if( forceRehash )
                {
                    for( zp_size_t i = 0; i < m_count; ++i )
                    {
                        if( entries[ i ].hash != nhash )
                        {
                            entries[ i ].hash = m_cmp.hash( entries[ i ].value );
                        }
                    }
                }

                for( zp_size_t i = 0; i < m_count; ++i )
                {
                    if( entries[ i ].hash != nhash )
                    {
                        const zp_size_t index = entries[ i ].hash % capacity;
                        entries[ i ].next = buckets[ index ];
                        buckets[ index ] = i;
                    }
                }

                m_allocator.free( m_entries );
                m_entries = nullptr;
            }

            if( m_buckets != nullptr )
            {
                m_allocator.free( m_buckets );
                m_buckets = nullptr;
            }

            m_entries = entries;
            m_buckets = buckets;

            m_capacity = capacity;
        }
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_size_t Set<T, H, Comparer, Allocator>::findIndex( const_reference value ) const
    {
        zp_size_t index = npos;

        if( m_buckets != nullptr )
        {
            const_hash_value hash = m_cmp.hash( value );
            for( zp_size_t b = m_buckets[ hash % m_capacity ]; b != npos; b = m_entries[ b ].next )
            {
                if( m_entries[ b ].hash == hash && m_cmp.equals( m_entries[ b ].value, value ) )
                {
                    index = b;
                    break;
                }
            }
        }

        return index;
    }

    //
    //
    //

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::SetIterator::SetIterator()
        : m_set( nullptr )
        , m_current( 0 )
    {
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::SetIterator::SetIterator( self_pointer set, zp_size_t index )
        : m_set( set )
        , m_current( index )
    {

    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::SetIterator::operator++()
    {
        zp_size_t idx = m_current + 1;
        while( idx < m_set->m_count && m_set->m_entries[ idx ].hash == nhash )
        {
            ++idx;
        }

        m_current = idx;

    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::SetIterator::operator++( int )
    {
        zp_size_t idx = m_current + 1;
        while( idx < m_set->m_count && m_set->m_entries[ idx ].hash == nhash )
        {
            ++idx;
        }

        m_current = idx;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::pointer Set<T, H, Comparer, Allocator>::SetIterator::operator->()
    {
        return &m_set->m_entries[ m_current ].value;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::const_pointer Set<T, H, Comparer, Allocator>::SetIterator::operator->() const
    {
        return &m_set->m_entries[ m_current ].value;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::const_reference Set<T, H, Comparer, Allocator>::SetIterator::operator*() const
    {
        return m_set->m_entries[ m_current ].value;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::SetIterator::operator==( const SetIterator& other ) const
    {
        return m_set == other.m_set && m_current == other.m_current;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::SetIterator::operator!=( const SetIterator& other ) const
    {
        return !( operator==( other ) );
    }

    //
    //
    //


    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::SetConstIterator::SetConstIterator()
        : m_set( nullptr )
        , m_current( 0 )
    {
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    Set<T, H, Comparer, Allocator>::SetConstIterator::SetConstIterator( const_self_pointer set, zp_size_t index )
        : m_set( set )
        , m_current( index )
    {

    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::SetConstIterator::operator++()
    {
        zp_size_t idx = m_current + 1;
        while( idx < m_set->m_count && m_set->m_entries[ idx ].hash == nhash )
        {
            ++idx;
        }

        m_current = idx;

    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    void Set<T, H, Comparer, Allocator>::SetConstIterator::operator++( int )
    {
        zp_size_t idx = m_current + 1;
        while( idx < m_set->m_count && m_set->m_entries[ idx ].hash == nhash )
        {
            ++idx;
        }

        m_current = idx;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::const_pointer Set<T, H, Comparer, Allocator>::SetConstIterator::operator->() const
    {
        return &m_set->m_entries[ m_current ].value;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    typename Set<T, H, Comparer, Allocator>::const_reference Set<T, H, Comparer, Allocator>::SetConstIterator::operator*() const
    {
        return m_set->m_entries[ m_current ].value;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::SetConstIterator::operator==( const SetConstIterator& other ) const
    {
        return m_set == other.m_set && m_current == other.m_current;
    }

    template<typename T, typename H, typename Comparer, typename Allocator>
    zp_bool_t Set<T, H, Comparer, Allocator>::SetConstIterator::operator!=( const SetConstIterator& other ) const
    {
        return !( operator==( other ) );
    }

}

#endif //ZP_SET_H
