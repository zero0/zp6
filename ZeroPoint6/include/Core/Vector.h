//
// Created by phosg on 1/25/2022.
//

#ifndef ZP_VECTOR_H
#define ZP_VECTOR_H

#include "Core/Defines.h"
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

    template<zp_size_t Length, typename T>
    struct FixedMemoryVectorAllocator
    {
        [[nodiscard]] void* allocate( zp_size_t size )
        {
            return static_cast<void*>( m_data );
        }

        void free( void* ptr ) const
        {
        }

        T m_data[ Length ];
    };
}

namespace zp
{
    template<typename T, typename Allocator = MemoryLabelAllocator>
    class Vector
    {
    public:
        typedef zp_bool_t (* EqualityComparerFunc)( const T& lh, const T& rh );

        typedef zp_bool_t (* ComparerFunc)( const T& value );

        using self_type = Vector<T, Allocator>;

        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T&& move_reference;
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

        //template<typename ... Args>
        //explicit Vector( Args ... args );

        template<zp_size_t Size>
        Vector( value_type (&array)[Size] );

        ~Vector();

        self_type& operator=( const self_type& other );

        self_type& operator=( self_type&& other ) noexcept;

        reference operator[]( zp_size_t index );

        const_reference operator[]( zp_size_t index ) const;

        const_reference at( zp_size_t index ) const;


        [[nodiscard]] zp_size_t length() const;

        [[nodiscard]] zp_size_t sizeAtomic() const;

        [[nodiscard]] zp_bool_t isEmpty() const;

        [[nodiscard]] zp_bool_t isEmptyAtomic() const;

        [[nodiscard]] zp_bool_t isFixed() const;

        void pushBack( const_reference val );

        void pushBack( move_reference val );

        void pushBackUnsafe( const_reference val );

        void pushBackUnsafe( move_reference val );

        void pushBackAtomic( const_reference val );

        void pushBackAtomic( move_reference val );

        reference pushBackEmpty();

        reference pushBackEmptyUnsafe();

        reference pushBackEmptyAtomic();

        zp_size_t pushBackEmptyRange( zp_size_t count, zp_bool_t initialize = true );

        zp_size_t pushBackEmptyRangeAtomic( zp_size_t count, zp_bool_t initialize = true );

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

        template<typename Cmp>
        zp_size_t findIndexOf( Cmp cmp ) const;

        zp_size_t findLastIndexOf( ComparerFunc cmp ) const;

        template<typename Cmp>
        void sort( Cmp cmp )
        {
            if( m_length > 1 )
            {
                zp_qsort3( begin(), end() - 1, cmp );
            }
        }

        template<typename Func>
        void foreach( Func func ) const
        {
            for( const_iterator b = begin(), e = end(); b != e; ++b )
            {
                func( *b );
            }
        }

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
        zp_size_t m_length;
        zp_size_t m_capacity;

        allocator_value m_allocator;
    };
}

namespace zp
{
    template<typename T, zp_size_t Size>
    using FixedVector = Vector<T, FixedMemoryVectorAllocator<Size, T>>;

}

//
// Impl
//

namespace zp
{
    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector()
        : m_data( nullptr )
        , m_length( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector( allocator_const_reference allocator )
        : m_data( nullptr )
        , m_length( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator )
    {
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector( zp_size_t capacity )
        : m_data( nullptr )
        , m_length( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
        ensureCapacity( capacity );
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::Vector( zp_size_t capacity, allocator_const_reference allocator )
        : m_data( nullptr )
        , m_length( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator )
    {
        ensureCapacity( capacity );
    }

    //template<typename T, typename Allocator>
    //template<typename ... Args>
    //Vector<T, Allocator>::Vector( Args ... args )
    //    : m_data( nullptr )
    //    , m_length( 0 )
    //    , m_capacity( 0 )
    //    , m_allocator( allocator_value() )
    //{
    //    ensureCapacity( sizeof...(args) );
    //    pushBack( zp_forward<T>( args... ) );
    //}

    template<typename T, typename Allocator>
    template<zp_size_t Size>
    Vector<T, Allocator>::Vector( value_type (&array)[Size] )
        : m_data( nullptr )
        , m_length( 0 )
        , m_capacity( 0 )
        , m_allocator( allocator_value() )
    {
        ensureCapacity( Size );
        for( auto& arg : array )
        {
            pushBack( arg );
        }
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::~Vector()
    {
        destroy();
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::self_type& Vector<T, Allocator>::operator=( const self_type& other )
    {
        clear();

        ensureCapacity( other.length() );

        m_length = other.m_length;

        for( zp_size_t i = 0; i != m_length; ++i )
        {
            m_data[ i ] = zp_move( other.m_data[ i ] );
        }

        return *this;
    }

    template<typename T, typename Allocator>
    Vector<T, Allocator>::self_type& Vector<T, Allocator>::operator=( self_type&& other ) noexcept
    {
        destroy();

        m_data  = other.m_data;
        m_length = other.m_length;
        m_capacity = other.m_capacity;

        other.m_data = nullptr;
        other.m_length = 0;
        other.m_capacity = 0;

        return *this;
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
    zp_size_t Vector<T, Allocator>::length() const
    {
        return m_length;
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::sizeAtomic() const
    {
        return Atomic::AddSizeT( m_length, 0 );
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isEmpty() const
    {
        return m_length == 0;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isEmptyAtomic() const
    {
        return Atomic::AddSizeT( m_length, 0 ) == 0;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isFixed() const
    {
        return m_allocator.isFixed();
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBack( const_reference val )
    {
        if( m_length == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }
        m_data[ m_length++ ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBack( move_reference val )
    {
        if( m_length == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }
        m_data[ m_length++ ] = zp_move( val );
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackUnsafe( const_reference val )
    {
        m_data[ m_length++ ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackUnsafe( move_reference val )
    {
        m_data[ m_length++ ] = zp_move( val );
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackAtomic( const_reference val )
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_length ) - 1;
        m_data[ index ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackAtomic( move_reference val )
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_length ) - 1;
        m_data[ index ] = zp_move( val );
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushBackEmpty()
    {
        if( m_length == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }
        new( m_data + m_length ) T();
        return m_data[ m_length++ ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushBackEmptyUnsafe()
    {
        new( m_data + m_length ) T();
        return m_data[ m_length++ ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushBackEmptyAtomic()
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_length ) - 1;

        new( m_data + index ) T();
        return m_data[ index ];
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::pushBackEmptyRange( zp_size_t count, zp_bool_t initialize )
    {
        const zp_size_t index = m_length;
        m_length += count;

        if( m_length >= m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }

        //if( initialize )
        //{
        //    for( zp_size_t i = index, imax = index + count; i < imax; ++i )
        //    {
        //        new( m_data + i ) T();
        //    }
        //}

        return index;
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::pushBackEmptyRangeAtomic( zp_size_t count, zp_bool_t initialize )
    {
        const zp_size_t index = Atomic::AddSizeT( &m_length, count ) - count;

        if( initialize )
        {
            for( zp_size_t i = index, imax = index + count; i < imax; ++i )
            {
                new( m_data + i ) T();
            }
        }

        return index;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushFront( const_reference val )
    {
        if( m_length == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }

        for( zp_size_t i = m_length + 1; i >= 1; --i )
        {
            m_data[ i ] = zp_move( m_data[ i - 1 ] );
        }

        ++m_length;

        m_data[ 0 ] = val;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::reference Vector<T, Allocator>::pushFrontEmpty()
    {
        if( m_length == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }

        for( zp_size_t i = m_length + 1; i >= 1; --i )
        {
            m_data[ i ] = zp_move( m_data[ i - 1 ] );
        }

        ++m_length;

        new( m_data ) T();
        return m_data[ 0 ];
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::popBack()
    {
        if( m_length )
        {
            ( m_data + --m_length )->~T();
        }
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::popFront()
    {
        if( m_length )
        {
            m_data->~T();

            for( zp_size_t i = 1; i < m_length; ++i )
            {
                m_data[ i - 1 ] = zp_move( m_data[ i ] );
            }

            --m_length;
        }
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::eraseAt( zp_size_t index )
    {
        ( m_data + index )->~T();

        for( zp_size_t i = index + 1; i < m_length; ++i )
        {
            m_data[ i - 1 ] = zp_move( m_data[ i ] );
        }

        --m_length;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::eraseAtSwapBack( zp_size_t index )
    {
        ( m_data + index )->~T();
        m_data[ index ] = zp_move( m_data[ m_length - 1 ] );
        --m_length;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::erase( const_reference val, EqualityComparerFunc comparer )
    {
        const zp_size_t index = indexOf( val, comparer );
        const zp_bool_t found = index != npos;

        if( found )
        {
            eraseAt( index );
        }

        return found;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::eraseSwapBack( const_reference val, EqualityComparerFunc comparer )
    {
        const zp_size_t index = indexOf( val, comparer );
        const zp_bool_t found = index != npos;

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

        for( zp_size_t i = 0; i < m_length; ++i )
        {
            T* t = m_data + i;
            if( cmp( *t, val ) )
            {
                ++numErased;

                t->~T();

                m_data[ i ] = zp_move( m_data[ m_length - 1 ] );
                --m_length;
                --i;
            }

        }

        return numErased;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::clear()
    {
        iterator b = begin();
        iterator e = end();
        for( ; b != e; ++b )
        {
            b->~T();
        }

        m_length = 0;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::reset()
    {
        m_length = 0;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::resize_unsafe( zp_size_t size )
    {
        ZP_ASSERT( size <= m_capacity );
        m_length = size;
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

        if( m_length > 0 )
        {
            EqualityComparerFunc cmp = comparer ? comparer : defaultEqualityComparerFunc;

            const_iterator b = m_data;
            const_iterator e = m_data + m_length;
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

        if( m_length > 0 )
        {
            EqualityComparerFunc cmp = comparer ? comparer : defaultEqualityComparerFunc;

            const_iterator b = begin();
            const_iterator e = end();
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
    template<typename Cmp>
    zp_size_t Vector<T, Allocator>::findIndexOf( Cmp cmp ) const
    {
        zp_size_t index = npos;

        if( m_length > 0 )
        {
            const_iterator b = m_data;
            const_iterator e = m_data + m_length;
            for( ; b != e; ++b )
            {
                if( cmp( *b ) )
                {
                    index = b - m_data;
                    break;
                }
            }
        }

        return index;
    }

    template<typename T, typename Allocator>
    zp_size_t Vector<T, Allocator>::findLastIndexOf( ComparerFunc cmp ) const
    {
        zp_size_t index = npos;

        if( m_length > 0 )
        {
            const_iterator b = begin();
            const_iterator e = end();
            for( ; b != e; )
            {
                --e;
                if( cmp( *e ) )
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
        return m_data[ m_length - 1 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_reference Vector<T, Allocator>::front() const
    {
        return m_data[ 0 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_reference Vector<T, Allocator>::back() const
    {
        return m_data[ m_length - 1 ];
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::iterator Vector<T, Allocator>::begin()
    {
        return m_data;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::iterator Vector<T, Allocator>::end()
    {
        return m_data + m_length;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_iterator Vector<T, Allocator>::begin() const
    {
        return m_data;
    }

    template<typename T, typename Allocator>
    typename Vector<T, Allocator>::const_iterator Vector<T, Allocator>::end() const
    {
        return m_data + m_length;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::ensureCapacity( zp_size_t capacity )
    {
        capacity = capacity < 4 ? 4 : capacity;

        pointer newArray = static_cast<pointer>( m_allocator.allocate( sizeof( T ) * capacity ));
        zp_zero_memory_array( newArray, capacity );

        if( m_data != nullptr )
        {
            for( zp_size_t i = 0; i != m_length; ++i )
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
