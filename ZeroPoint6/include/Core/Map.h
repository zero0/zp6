//
// Created by phosg on 2/23/2022.
//

#ifndef ZP_MAP_H
#define ZP_MAP_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

namespace zp
{
    template<typename Key, typename Value, typename H = zp_hash64_t, typename Comparer = EqualityComparer<Key, H>, typename Allocator = MemoryLabelAllocator>
    class Map
    {
    public:
        static const zp_size_t npos = -1;
        static constexpr H nhash = zp_fnv_1a<H>( nullptr, 0 );

        typedef Key key_value;
        typedef Key& key_reference;
        typedef const Key& key_const_reference;

        typedef Value value_value;
        typedef Value& value_reference;
        typedef const Value& value_const_reference;

        typedef Value* value_pointer;
        typedef Value*& value_pointer_reference;
        typedef Value** value_pointer_pointer;
        typedef const Value* value_const_pointer;

        typedef Allocator allocator_value;
        typedef const Allocator& allocator_const_reference;

        typedef Comparer comparer_value;
        typedef const Comparer& comparer_const_reference;

        typedef H hash_value;
        typedef const H const_hash_value;

        class MapIterator;

        class MapConstIterator;

        typedef MapIterator iterator;
        typedef MapConstIterator const_iterator;

        typedef Map<Key, Value, H, Comparer, Allocator> self_value;
        typedef const Map<Key, Value, H, Comparer, Allocator>& const_self_reference;
        typedef const Map<Key, Value, H, Comparer, Allocator>* const_self_pointer;
        typedef Map<Key, Value, H, Comparer, Allocator>* self_pointer;

        Map();

        explicit Map( zp_size_t capacity );

        Map( zp_size_t capacity, comparer_const_reference cmp );

        Map( zp_size_t capacity, allocator_const_reference allocator );

        Map( zp_size_t capacity, comparer_const_reference cmp, allocator_const_reference allocator );

        explicit Map( comparer_const_reference cmp );

        Map( comparer_const_reference cmp, allocator_const_reference allocator );

        explicit Map( allocator_const_reference allocator );

        ~Map();

        [[nodiscard]] zp_size_t size() const;

        [[nodiscard]] zp_bool_t isEmpty() const;

        zp_bool_t get( key_const_reference key, value_reference value ) const;

        zp_bool_t get( key_const_reference key, value_pointer value ) const;

        zp_bool_t get( key_const_reference key, value_pointer_reference value ) const;

        zp_bool_t get( key_const_reference key, value_pointer_pointer value ) const;

        void set( key_const_reference key, value_const_reference value );

        void setAll( const_self_reference other );

        zp_bool_t containsKey( key_const_reference key ) const;

        zp_bool_t remove( key_const_reference key );

        void reserve( zp_size_t size );

        void clear();

        void destroy();

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;

    private:
        void ensureCapacity( zp_size_t capacity, zp_bool_t forceRehash );

        zp_size_t findIndex( key_const_reference key ) const;

        struct MapEntry
        {
            hash_value hash;
            zp_size_t next;
            key_value key;
            value_value value;
        };

        MapEntry* m_entries;
        zp_size_t* m_buckets;

        zp_size_t m_capacity;
        zp_size_t m_count;
        zp_size_t m_freeCount;
        zp_size_t m_freeList;

        comparer_value m_cmp;
        allocator_value m_allocator;

    public:
        class MapIterator
        {
        private:
            MapIterator();

            MapIterator( self_pointer map, zp_size_t index );

        public:
            ~MapIterator();

            void operator++();

            void operator++( int );

            key_const_reference key() const;

            value_reference value();

            value_const_reference value() const;

            zp_bool_t operator==( const MapIterator& other ) const;

            zp_bool_t operator!=( const MapIterator& other ) const;

        private:
            self_pointer m_map;
            zp_size_t m_next;
            zp_size_t m_current;

            friend class Map;
        };

        class MapConstIterator
        {
        private:
            MapConstIterator();

            MapConstIterator( const_self_pointer map, zp_size_t index );

        public:
            ~MapConstIterator();

            void operator++();

            void operator++( int );

            key_const_reference key() const;

            value_const_reference value() const;

            zp_bool_t operator==( const MapConstIterator& other ) const;

            zp_bool_t operator!=( const MapConstIterator& other ) const;

        private:
            const_self_pointer m_map;
            zp_size_t m_next;
            zp_size_t m_current;

            friend class Map;
        };
    };
}

//
//
//

namespace zp
{
    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map()
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( zp_size_t capacity )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( zp_size_t capacity, comparer_const_reference cmp )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( zp_size_t capacity, allocator_const_reference allocator )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( zp_size_t capacity, comparer_const_reference cmp, allocator_const_reference allocator )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( comparer_const_reference cmp )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( comparer_const_reference cmp, allocator_const_reference allocator )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::Map( allocator_const_reference allocator )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::~Map()
    {
        destroy();
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_size_t Map<Key, Value, H, Comparer, Allocator>::size() const
    {
        const zp_size_t count = m_count - m_freeCount;
        return count;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::isEmpty() const
    {
        const zp_bool_t empty = m_count == m_freeCount;
        return empty;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::get( key_const_reference key, value_reference value ) const
    {
        const zp_size_t index = findIndex( key );
        const zp_bool_t found = index != npos;
        if( found )
        {
            value = m_entries[ index ].value;
        }

        return found;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::get( key_const_reference key, value_pointer value ) const
    {
        const zp_size_t index = findIndex( key );
        const zp_bool_t found = index != npos;
        if( found )
        {
            *value = m_entries[ index ].value;
        }

        return found;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::get( key_const_reference key, value_pointer_reference value ) const
    {
        const zp_size_t index = findIndex( key );
        const zp_bool_t found = index != npos;
        if( found )
        {
            value = &m_entries[ index ].value;
        }

        return found;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::get( key_const_reference key, value_pointer_pointer value ) const
    {
        const zp_size_t index = findIndex( key );
        const zp_bool_t found = index != npos;
        if( found )
        {
            *value = &m_entries[ index ].value;
        }

        return found;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::set( key_const_reference key, value_const_reference value )
    {
        if( m_buckets == nullptr )
        {
            ensureCapacity( 4, false );
        }

        const_hash_value h = m_cmp.hash( key );
        zp_size_t index = h % m_capacity;
        zp_size_t numSteps = 0;

        for( zp_size_t b = m_buckets[ index ]; b != npos; b = m_entries[ b ].next )
        {
            if( m_entries[ b ].hash == h && m_cmp.equals( m_entries[ b ].key, key ) )
            {
                m_entries[ b ].value = zp_move( value );
                return;
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
                index = h % m_capacity;
            }
            newIndex = m_count;
            ++m_count;
        }

        m_entries[ newIndex ].hash = h;
        m_entries[ newIndex ].next = m_buckets[ index ];
        m_entries[ newIndex ].key = zp_move( key );
        m_entries[ newIndex ].value = zp_move( value );

        m_buckets[ index ] = newIndex;

        if( numSteps >= m_capacity )
        {
            ensureCapacity( m_capacity + 1, true );
        }
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::setAll( const_self_reference other )
    {
        const_iterator b = other.begin();
        const_iterator e = other.begin();
        for( ; b != e; ++b )
        {
            set( b.key(), b.value() );
        }
    }


    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::containsKey( key_const_reference key ) const
    {
        const zp_size_t index = findIndex( key );
        return index != npos;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::remove( key_const_reference key )
    {
        zp_bool_t removed = false;

        if( m_buckets != nullptr )
        {
            const_hash_value h = m_cmp.hash( key );
            const zp_size_t index = h % m_capacity;
            zp_size_t removeIndex = npos;

            for( zp_size_t b = m_buckets[ index ]; b != npos; b = m_entries[ b ].next )
            {
                if( m_entries[ b ].hash == h && m_cmp.equals( m_entries[ b ].key, key ) )
                {
                    if( removeIndex == npos )
                    {
                        m_buckets[ index ] = m_entries[ b ].next;
                    }
                    else
                    {
                        m_entries[ removeIndex ].next = m_entries[ b ].next;
                    }

                    m_entries[ b ].hash = nhash;
                    m_entries[ b ].next = m_freeList;
                    ( &m_entries[ b ].key )->~key_value();
                    ( &m_entries[ b ].value )->~value_value();

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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::reserve( zp_size_t size )
    {
        ensureCapacity( size, false );
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::clear()
    {
        if( m_count > 0 )
        {
            for( zp_size_t i = 0; i < m_capacity; ++i )
            {
                m_buckets[ i ] = npos;
            }

            for( zp_size_t i = 0; i < m_count; ++i )
            {
                ( m_entries + i )->~MapEntry();
            }

            m_freeList = npos;
            m_freeCount = 0;
            m_count = 0;
        }
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::destroy()
    {
        clear();

        m_allocator.free( m_buckets );
        m_buckets = nullptr;

        m_allocator.free( m_entries );
        m_entries = nullptr;

        m_capacity = 0;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::iterator Map<Key, Value, H, Comparer, Allocator>::begin()
    {
        zp_size_t index = 0;
        while( index < m_count )
        {
            if( m_entries[ index ].hash != nhash )
            {
                break;
            }
            ++index;
        }

        return iterator( this, index );
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::iterator Map<Key, Value, H, Comparer, Allocator>::end()
    {
        return iterator( this, m_count );
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::const_iterator Map<Key, Value, H, Comparer, Allocator>::begin() const
    {
        zp_size_t index = 0;
        while( index < m_count )
        {
            if( m_entries[ index ].hash != nhash )
            {
                break;
            }
            ++index;
        }

        return const_iterator( this, index );
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::const_iterator Map<Key, Value, H, Comparer, Allocator>::end() const
    {
        return const_iterator( this, m_count );
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::ensureCapacity( zp_size_t capacity, zp_bool_t forceRehash )
    {
        if( capacity > m_capacity )
        {
            zp_size_t* buckets = static_cast<zp_size_t*>( m_allocator.allocate( sizeof( zp_size_t ) * capacity ) );
            MapEntry* entries = static_cast<MapEntry*>( m_allocator.allocate( sizeof( MapEntry ) * capacity ) );
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
                            entries[ i ].hash = m_cmp.hash( entries[ i ].key );
                        }
                    }
                }

                for( zp_size_t i = 0; i < m_count; ++i )
                {
                    if( entries[ i ].hash != nhash )
                    {
                        zp_size_t index = entries[ i ].hash % capacity;
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_size_t Map<Key, Value, H, Comparer, Allocator>::findIndex( key_const_reference key ) const
    {
        zp_size_t index = npos;

        if( m_buckets != nullptr )
        {
            const_hash_value h = m_cmp.hash( key );
            const zp_size_t idx = h % m_capacity;
            for( zp_size_t b = m_buckets[ idx ]; b != npos; b = m_entries[ b ].next )
            {
                if( m_entries[ b ].hash == h && m_cmp.equals( m_entries[ b ].key, key ) )
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

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::MapIterator::MapIterator()
        : m_map( nullptr )
        , m_next( 0 )
        , m_current( 0 )
    {
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::MapIterator::MapIterator( self_pointer map, zp_size_t index )
        : m_map( map )
        , m_next( index + 1 )
        , m_current( index )
    {
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::MapIterator::~MapIterator()
    {
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::MapIterator::operator++()
    {
        while( m_next < m_map->m_count )
        {
            if( m_map->m_entries[ m_next ].hash != nhash )
            {
                m_current = m_next;
                ++m_next;
                return;
            }
            ++m_next;
        }

        m_next = m_map->m_count + 1;
        m_current = m_map->m_count;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::MapIterator::operator++( int )
    {
        operator++();
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::key_const_reference Map<Key, Value, H, Comparer, Allocator>::MapIterator::key() const
    {
        return m_map->m_entries[ m_current ].key;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::value_reference Map<Key, Value, H, Comparer, Allocator>::MapIterator::value()
    {
        return m_map->m_entries[ m_current ].value;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::value_const_reference Map<Key, Value, H, Comparer, Allocator>::MapIterator::value() const
    {
        return m_map->m_entries[ m_current ].value;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::MapIterator::operator==( const MapIterator& other ) const
    {
        return m_map == other.m_map && m_current == other.m_current;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::MapIterator::operator!=( const MapIterator& other ) const
    {
        return !( operator==( other ) );
    }

//
//
//

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::MapConstIterator()
        : m_map( nullptr )
        , m_next( 0 )
        , m_current( 0 )
    {
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::MapConstIterator( const_self_pointer map, zp_size_t index )
        : m_map( map )
        , m_next( index + 1 )
        , m_current( index )
    {
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::~MapConstIterator()
    {
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::operator++()
    {
        while( m_next < m_map->m_count )
        {
            if( m_map->m_entries[ m_next ].hash != nhash )
            {
                m_current = m_next;
                ++m_next;
                return;
            }
            ++m_next;
        }

        m_next = m_map->m_count + 1;
        m_current = m_map->m_count;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    void Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::operator++( int )
    {
        operator++();
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::key_const_reference Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::key() const
    {
        return m_map->m_entries[ m_current ].key;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    typename Map<Key, Value, H, Comparer, Allocator>::value_const_reference Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::value() const
    {
        return m_map->m_entries[ m_current ].value;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::operator==( const MapConstIterator& other ) const
    {
        return m_map == other.m_map && m_current == other.m_current;
    }

    template<typename Key, typename Value, typename H, typename Comparer, typename Allocator>
    zp_bool_t Map<Key, Value, H, Comparer, Allocator>::MapConstIterator::operator!=( const MapConstIterator& other ) const
    {
        return !( operator==( other ) );
    }
}

#endif //ZP_MAP_H
