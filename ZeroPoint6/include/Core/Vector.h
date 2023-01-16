//
// Created by phosg on 1/25/2022.
//

#ifndef ZP_VECTOR_H
#define ZP_VECTOR_H

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Allocator.h"
#include "Core/Atomic.h"

namespace zp
{
    template<MemoryLabel MemLabel, zp_size_t Alignment = kDefaultMemoryAlignment>
    struct TypedMemoryLabelAllocator
    {
        [[nodiscard]] void* allocate( zp_size_t size ) const
        {
            void* ptr = GetAllocator( MemLabel )->allocate( size, Alignment );
            return ptr;
        }

        void free( void* ptr ) const
        {
            GetAllocator( MemLabel )->free( ptr );
        }
    };


    template<typename T, typename Allocator = MemoryLabelAllocator>
    class Vector
    {
    public:
        typedef zp_bool_t (* EqualityComparerFunc)( const T& lh, const T& rh );

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

        Vector();

        explicit Vector( allocator_const_reference allocator );

        explicit Vector( zp_size_t capacity );

        Vector( zp_size_t capacity, allocator_const_reference allocator );

        ~Vector();

        reference operator[]( zp_size_t index );

        const_reference operator[]( zp_size_t index ) const;

        const_reference at( zp_size_t index ) const;

        [[nodiscard]] zp_size_t size() const;

        [[nodiscard]] zp_bool_t isEmpty() const;

        [[nodiscard]] zp_bool_t isFixed() const;

        void pushBack( const_reference val );

        void pushBackUnsafe( const_reference val );

        void pushBackAtomic( const_reference val );

        reference pushBackEmpty();

        reference pushBackEmptyUnsafe();

        reference pushBackEmptyAtomic();

        void pushFront( const_reference val );

        reference pushFrontEmpty();

        void popBack();

        void popFront();

        void eraseAt( zp_size_t index );

        void eraseAtSwapBack( zp_size_t index );

        zp_bool_t erase( const_reference val, EqualityComparerFunc comparer = nullptr );

        zp_bool_t eraseSwapBack( const_reference val, EqualityComparerFunc comparer = nullptr );

        zp_size_t eraseAll( const_reference val, EqualityComparerFunc comparer = nullptr );

        void clear();

        void reset();

        void resize_unsafe( zp_size_t size );

        void reserve( zp_size_t size );

        void destroy();

        zp_size_t indexOf( const_reference val, EqualityComparerFunc comparer = nullptr ) const;

        zp_size_t lastIndexOf( const_reference val, EqualityComparerFunc comparer = nullptr ) const;

        pointer data();

        const_pointer data() const;

        T& front();

        T& back();

        const_reference front() const;

        const_reference back() const;

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;

    private:
        void ensureCapacity( zp_size_t capacity );

        static zp_bool_t defaultEqualityComparerFunc( const T& lh, const T& rh );

        pointer m_data;
        zp_size_t m_size;
        zp_size_t m_capacity;

        allocator_value m_allocator;
    };
};

//
// Impl
//

namespace zp
{
    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector()
        : m_data( nullptr )
        , m_size( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector( allocator_const_reference allocator )
        : m_data( nullptr )
        , m_size( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator )
    {
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector( zp_size_t capacity )
        : m_data( nullptr )
        , m_size( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
        ensureCapacity( capacity );
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector( zp_size_t capacity, allocator_const_reference allocator )
        : m_data( nullptr )
        , m_size( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator )
    {
        ensureCapacity( capacity );
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::~Vector()
    {
        destroy();
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::operator[]( zp_size_t index )
    {
        return m_data[ index ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_reference Vector<T, Allocator>::operator[]( zp_size_t index ) const
    {
        return m_data[ index ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_reference Vector<T, Allocator>::at( zp_size_t index ) const
    {
        return m_data[ index ];
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::size() const
    {
        return m_size;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isEmpty() const
    {
        return m_size == 0;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isFixed() const
    {
        return m_allocator.isFixed();
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBack( const_reference val )
    {
        if( m_size == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }
        m_data[ m_size++ ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackUnsafe( const_reference val )
    {
        m_data[ m_size++ ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackAtomic( const_reference val )
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_size ) - 1;
        m_data[ index ] = val;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushBackEmpty()
    {
        if( m_size == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }
        new( m_data + m_size ) T();
        return m_data[ m_size++ ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushBackEmptyUnsafe()
    {
        new( m_data + m_size ) T();
        return m_data[ m_size++ ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushBackEmptyAtomic()
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_size ) - 1;

        new( m_data + index ) T();
        return m_data[ index ];
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushFront( const_reference val )
    {
        if( m_size == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }

        for( zp_size_t i = m_size + 1; i >= 1; --i )
        {
            m_data[ i ] = zp_move( m_data[ i - 1 ] );
        }

        ++m_size;

        m_data[ 0 ] = val;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushFrontEmpty()
    {
        if( m_size == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }

        for( zp_size_t i = m_size + 1; i >= 1; --i )
        {
            m_data[ i ] = zp_move( m_data[ i - 1 ] );
        }

        ++m_size;

        new( m_data ) T();
        return m_data[ 0 ];
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::popBack()
    {
        if( m_size )
        {
            ( m_data + --m_size )->~T();
        }
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::popFront()
    {
        if( m_size )
        {
            m_data->~T();

            for( zp_size_t i = 1; i < m_size; ++i )
            {
                m_data[ i - 1 ] = zp_move( m_data[ i ] );
            }

            --m_size;
        }
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::eraseAt( zp_size_t index )
    {
        ( m_data + index )->~T();

        for( zp_size_t i = index + 1; i < m_size; ++i )
        {
            m_data[ i - 1 ] = zp_move( m_data[ i ] );
        }

        --m_size;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::eraseAtSwapBack( zp_size_t index )
    {
        ( m_data + index )->~T();
        m_data[ index ] = zp_move( m_data[ m_size - 1 ] );
        --m_size;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::erase( const_reference val, EqualityComparerFunc comparer )
    {
        zp_size_t index = indexOf( val, comparer );
        zp_bool_t found = index != npos;

        if( found )
        {
            eraseAt( index );
        }

        return found;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::eraseSwapBack( const_reference val, EqualityComparerFunc comparer )
    {
        zp_size_t index = indexOf( val, comparer );
        zp_bool_t found = index != npos;

        if( found )
        {
            eraseAtSwapBack( index );
        }

        return found;
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::eraseAll( const_reference val, EqualityComparerFunc comparer )
    {
        zp_size_t numErased = 0;

        EqualityComparerFunc cmp = comparer ? comparer : defaultEqualityComparerFunc;

        for( zp_size_t i = 0; i < m_size; ++i )
        {
            T* t = m_data + i;
            if( cmp( *t, val ) )
            {
                ++numErased;

                t->~T();

                m_data[ i ] = zp_move( m_data[ m_size - 1 ] );
                --m_size;
                --i;
            }

        }

        return numErased;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::clear()
    {
        iterator b = m_data;
        iterator e = m_data + m_size;
        for( ; b != e; ++b )
        {
            b->~T();
        }

        m_size = 0;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::reset()
    {
        m_size = 0;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::resize_unsafe( zp_size_t size )
    {
        ZP_ASSERT( size <= m_capacity );
        m_size = size;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::reserve( zp_size_t size )
    {
        if( size > m_capacity )
        {
            ensureCapacity( size );
        }
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::destroy()
    {
        clear();

        if( m_data )
        {
            m_allocator.free( m_data );
            m_data = nullptr;
        }

        m_capacity = 0;
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::indexOf( const_reference val, EqualityComparerFunc comparer ) const
    {
        zp_size_t index = npos;

        if( m_size > 0 )
        {
            EqualityComparerFunc cmp = comparer ? comparer : defaultEqualityComparerFunc;

            const_iterator b = m_data;
            const_iterator e = m_data + m_size;
            for( ; b != e; ++b )
            {
                if( cmp( *b, val ) )
                {
                    index = b - m_data;
                    break;
                }
            }
        }

        return index;
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::lastIndexOf( const_reference val, EqualityComparerFunc comparer ) const
    {
        zp_size_t index = npos;

        if( m_size > 0 )
        {
            EqualityComparerFunc cmp = comparer ? comparer : defaultEqualityComparerFunc;

            const_iterator b = m_data;
            const_iterator e = m_data + m_size;
            for( ; b != e; )
            {
                --e;
                if( cmp( *e, val ) )
                {
                    index = e - m_data;
                    break;
                }
            }
        }

        return index;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::pointer Vector<T, Allocator>::data()
    {
        return m_data;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_pointer Vector<T, Allocator>::data() const
    {
        return m_data;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::front()
    {
        return m_data[ 0 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::back()
    {
        return m_data[ m_size - 1 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_reference Vector<T, Allocator>::front() const
    {
        return m_data[ 0 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_reference Vector<T, Allocator>::back() const
    {
        return m_data[ m_size - 1 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::iterator Vector<T, Allocator>::begin()
    {
        return m_data;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::iterator Vector<T, Allocator>::end()
    {
        return m_data + m_size;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_iterator Vector<T, Allocator>::begin() const
    {
        return m_data;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_iterator Vector<T, Allocator>::end() const
    {
        return m_data + m_size;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::ensureCapacity( zp_size_t capacity )
    {
        capacity = capacity < 4 ? 4 : capacity;

        pointer newArray = static_cast<pointer>( m_allocator.allocate( sizeof( T ) * capacity ));
        zp_zero_memory_array( newArray, capacity );

        if( m_data != nullptr )
        {
            for( zp_size_t i = 0; i != m_size; ++i )
            {
                newArray[ i ] = zp_move( m_data[ i ] );
            }

            m_allocator.free( m_data );
            m_data = nullptr;
        }

        m_data = newArray;
        m_capacity = capacity;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::defaultEqualityComparerFunc( const T& lh, const T& rh )
    {
        return lh == rh;
    }
};

#endif //ZP_VECTOR_H
