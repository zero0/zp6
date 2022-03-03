//
// Created by phosg on 2/23/2022.
//

#ifndef ZP_QUEUE_H
#define ZP_QUEUE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Allocator.h"

namespace zp
{
    template<typename T, typename Allocator = MemoryLabelAllocator>
    class Queue
    {
    public:
        static const zp_size_t npos = -1;

        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;

        typedef T* iterator;
        typedef const T* const_iterator;

        typedef Allocator allocator_value;
        typedef const allocator_value& allocator_const_reference;

        Queue();

        explicit Queue( allocator_const_reference allocator );

        explicit Queue( zp_size_t capacity );

        Queue( zp_size_t capacity, allocator_const_reference allocator );

        ~Queue();

        zp_size_t size() const;

        zp_bool_t isEmpty() const;

        void enqueue( const_reference val );

        value_type dequeue();

        zp_bool_t tryDequeue( reference val );

        const_reference peek() const;

        void clear();

        void reset();

        void reserve( zp_size_t size );

        void destroy();

    private:
        void ensureCapacity( zp_size_t capacity );

        pointer m_data;
        zp_size_t m_size;
        zp_size_t m_head;
        zp_size_t m_tail;
        zp_size_t m_capacity;

        allocator_value m_allocator;
    };
}

//
//
//

namespace zp
{
    template<typename T, typename Allocator>
    Queue<T, Allocator>::Queue()
        : m_data( nullptr )
        , m_size( 0 )
        , m_head( 0 )
        , m_tail( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
    }

    template<typename T, typename Allocator>
    Queue<T, Allocator>::Queue( allocator_const_reference allocator )
        : m_data( nullptr )
        , m_size( 0 )
        , m_head( 0 )
        , m_tail( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator )
    {
    }

    template<typename T, typename Allocator>
    Queue<T, Allocator>::Queue( zp_size_t capacity )
        : m_data( nullptr )
        , m_size( 0 )
        , m_head( 0 )
        , m_tail( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
        ensureCapacity( capacity );
    }

    template<typename T, typename Allocator>
    Queue<T, Allocator>::Queue( zp_size_t capacity, allocator_const_reference allocator )
        : m_data( nullptr )
        , m_size( 0 )
        , m_head( 0 )
        , m_tail( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator )
    {
        ensureCapacity( capacity );
    }

    template<typename T, typename Allocator>
    Queue<T, Allocator>::~Queue()
    {
        destroy();
    }

    template<typename T, typename Allocator>
    zp_size_t Queue<T, Allocator>::size() const
    {
        return m_size;
    }

    template<typename T, typename Allocator>
    zp_bool_t Queue<T, Allocator>::isEmpty() const
    {
        return m_size == 0;
    }

    template<typename T, typename Allocator>
    void Queue<T, Allocator>::enqueue( const_reference val )
    {
        if( m_size == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }

        m_data[ m_tail ] = val;

        m_tail = ( m_tail + 1 ) % m_capacity;
        ++m_size;
    }

    template<typename T, typename Allocator>
    typename Queue<T, Allocator>::value_type Queue<T, Allocator>::dequeue()
    {
        ZP_ASSERT_MSG( m_size, "Trying to dequeue from an empty Queue" );

        pointer p = m_data + m_head;

        value_type v = zp_move( *p );
        p->~T();

        m_head = ( m_head + 1 ) % m_capacity;
        --m_size;

        return zp_move( v );
    }

    template<typename T, typename Allocator>
    zp_bool_t Queue<T, Allocator>::tryDequeue( reference val )
    {
        zp_bool_t removed = false;
        if( m_size > 0 )
        {
            pointer p = m_data + m_head;

            val = zp_move( *p );
            p->~T();

            m_head = ( m_head + 1 ) % m_capacity;
            --m_size;

            removed = true;
        }

        return removed;
    }

    template<typename T, typename Allocator>
    typename Queue<T, Allocator>::const_reference Queue<T, Allocator>::peek() const
    {
        return m_data[ m_tail ];
    }

    template<typename T, typename Allocator>
    void Queue<T, Allocator>::clear()
    {
        if( m_head < m_tail )
        {
            for( zp_size_t i = m_head, imax = m_tail; i < imax; ++i )
            {
                ( m_data + i )->~T();
            }
        }
        else if( m_size > 0 )
        {
            for( zp_size_t i = m_head, imax = m_capacity; i < imax; ++i )
            {
                ( m_data + i )->~T();
            }
            for( zp_size_t i = 0, imax = m_tail; i < imax; ++i )
            {
                ( m_data + i )->~T();
            }
        }

        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    template<typename T, typename Allocator>
    void Queue<T, Allocator>::reset()
    {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    template<typename T, typename Allocator>
    void Queue<T, Allocator>::reserve( zp_size_t size )
    {
        if( size > m_capacity )
        {
            ensureCapacity( size );
        }
    }

    template<typename T, typename Allocator>
    void Queue<T, Allocator>::destroy()
    {
        clear();

        if( m_data != nullptr )
        {
            m_allocator.free( m_data );
            m_data = nullptr;
        }

        m_capacity = 0;
    }

    template<typename T, typename Allocator>
    void Queue<T, Allocator>::ensureCapacity( zp_size_t capacity )
    {
        if( capacity < 4 )
        {
            capacity = 4;
        }

        pointer newData = static_cast<pointer>( m_allocator.allocate( sizeof( T ) * capacity ) );
        zp_zero_memory_array( newData, capacity );

        if( m_data != nullptr )
        {
            if( m_head < m_tail )
            {
                for( zp_size_t i = m_head, imax = m_tail, n = 0; i < imax; ++i, ++n )
                {
                    newData[ n ] = zp_move( m_data[ i ] );
                }
            }
            else if( m_size > 0 )
            {
                zp_size_t n = 0;
                for( zp_size_t i = m_head, imax = m_capacity; i < imax; ++i, ++n )
                {
                    newData[ n ] = zp_move( m_data[ i ] );
                }
                for( zp_size_t i = 0, imax = m_tail; i < imax; ++i, ++n )
                {
                    newData[ n ] = zp_move( m_data[ i ] );
                }
            }

            m_head = 0;
            m_tail = m_size;

            m_allocator.free( m_data );
            m_data = nullptr;
        }

        m_data = newData;
        m_capacity = capacity;
    }
}

#endif //ZP_QUEUE_H
