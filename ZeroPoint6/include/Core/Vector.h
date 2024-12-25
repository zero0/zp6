//
// Created by phosg on 1/25/2022.
//

#ifndef ZP_VECTOR_H
#define ZP_VECTOR_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Math.h"
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

        typedef zp_bool_t (* ComparerFunc)( const T& value );

        static const zp_size_t npos = -1;

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

        ~Vector();

        reference operator[]( zp_size_t index );

        const_reference operator[]( zp_size_t index ) const;

        const_reference at( zp_size_t index ) const;

        [[nodiscard]] zp_size_t size() const;

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
            if( m_size > 1 )
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
        zp_size_t m_size;
        zp_size_t m_capacity;

        allocator_value m_allocator;
    };
}

namespace zp
{
    template<typename T, typename Allocator = MemoryLabelAllocator>
    struct AutoMemory
    {
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T&& move_reference;
        typedef zp_remove_pointer_t<T>* pointer;
        typedef const zp_remove_pointer_t<T>* const_pointer;
        typedef T* iterator;
        typedef const T* const_iterator;

        typedef Allocator allocator_value;
        typedef const allocator_value& allocator_const_reference;

        explicit AutoMemory( zp_size_t size )
            : m_allocator( allocator_value() )
            , m_ptr( m_allocator.allocate( size ) )
            , m_size( size )
        {
        }

        explicit AutoMemory( zp_size_t size, allocator_const_reference allocator )
            : m_allocator( allocator )
            , m_ptr( m_allocator.allocate( size ) )
            , m_size( size )
        {
        }

        ~AutoMemory()
        {
            m_allocator.free( m_ptr );
        }

        [[nodiscard]] ZP_FORCEINLINE pointer ptr()
        {
            return m_ptr;
        }

        [[nodiscard]] ZP_FORCEINLINE const_pointer ptr() const
        {
            return m_ptr;
        }

        [[nodiscard]] ZP_FORCEINLINE zp_size_t size() const
        {
            return m_size;
        }

    private:
        allocator_value m_allocator;
        pointer m_ptr;
        zp_size_t m_size;
    };
}

namespace zp
{
    template<typename T>
    struct MemoryArray
    {
    public:
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T* iterator;
        typedef const T* const_iterator;

    public:
        [[nodiscard]] ZP_FORCEINLINE zp_bool_t isEmpty() const;

        [[nodiscard]] ZP_FORCEINLINE reference operator[]( zp_size_t index );

        [[nodiscard]] ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE iterator begin();

        [[nodiscard]] ZP_FORCEINLINE iterator end();

        [[nodiscard]] ZP_FORCEINLINE const_iterator begin() const;

        [[nodiscard]] ZP_FORCEINLINE const_iterator end() const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray split( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray split( zp_size_t index, zp_size_t count ) const;

    public:
        T* ptr;
        zp_size_t length;
    };
}

namespace zp
{
    template<typename T>
    struct MemoryList
    {
    public:
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T* iterator;
        typedef const T* const_iterator;

    public:
        MemoryList() = default;

        MemoryList( void* ptr, zp_size_t capacity );

        explicit MemoryList( Memory memory );

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t isEmpty() const;

        [[nodiscard]] ZP_FORCEINLINE zp_size_t length() const;

        [[nodiscard]] ZP_FORCEINLINE zp_size_t capacity() const;

        [[nodiscard]] ZP_FORCEINLINE reference operator[]( zp_size_t index );

        [[nodiscard]] ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const;

        ZP_FORCEINLINE void push_back( const T& value );

        ZP_FORCEINLINE void push_back( T&& value );

        ZP_FORCEINLINE T& push_back_empty();

        ZP_FORCEINLINE T& push_back_uninitialized();

        [[nodiscard]] ZP_FORCEINLINE iterator begin();

        [[nodiscard]] ZP_FORCEINLINE iterator end();

        [[nodiscard]] ZP_FORCEINLINE const_iterator begin() const;

        [[nodiscard]] ZP_FORCEINLINE const_iterator end() const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray<T> split( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray<T> split( zp_size_t index, zp_size_t count ) const;

    private:
        T* m_ptr;
        zp_size_t m_length;
        zp_size_t m_capacity;
    };
}

namespace zp
{
    template<typename T>
    class ReadonlyMemoryArray
    {
    public:
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef const T* const_pointer;
        typedef const T* const_iterator;

    public:
        ReadonlyMemoryArray() = default;

        ~ReadonlyMemoryArray() = default;

        ReadonlyMemoryArray( const_pointer ptr, zp_size_t length )
            : m_ptr( ptr )
            , m_length( length )
        {
        }

        [[nodiscard]] ZP_FORCEINLINE zp_size_t length() const
        {
            return m_length;
        }

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t empty() const
        {
            return m_length == 0;
        }

        [[nodiscard]] ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const
        {
            ZP_ASSERT( index < m_length );
            return m_ptr[ index ];
        }

        [[nodiscard]] ZP_FORCEINLINE const_iterator begin() const
        {
            return m_ptr;
        }

        [[nodiscard]] ZP_FORCEINLINE const_iterator end() const
        {
            return m_ptr + m_length;
        }

        [[nodiscard]] ZP_FORCEINLINE ReadonlyMemoryArray split( zp_size_t index ) const
        {
            return ReadonlyMemoryArray<T>( m_ptr + index, m_length - index );
        }

        [[nodiscard]] ZP_FORCEINLINE ReadonlyMemoryArray split( zp_size_t index, zp_size_t length ) const
        {
            return ReadonlyMemoryArray<T>( m_ptr + index, length );
        }

    private:
        const_pointer m_ptr;
        zp_size_t m_length;
    };
}

namespace zp
{
    template<typename T, zp_size_t Size>
    class FixedArray
    {
    public:
        typedef T value_type;
        typedef T& reference;
        typedef T&& move_type;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T* iterator;
        typedef const T* const_iterator;

    public:
        FixedArray();

        explicit FixedArray( value_type (& ptr)[Size] );

        template<typename ... Args>
        FixedArray( Args ... m );

        FixedArray( pointer ptr, zp_size_t length );

        [[nodiscard]] ZP_FORCEINLINE zp_size_t length() const;

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t empty() const;

        [[nodiscard]] ZP_FORCEINLINE reference operator[]( zp_size_t index );

        [[nodiscard]] ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE pointer data();

        [[nodiscard]] ZP_FORCEINLINE const_pointer data() const;

        [[nodiscard]] ZP_FORCEINLINE iterator begin();

        [[nodiscard]] ZP_FORCEINLINE iterator end();

        [[nodiscard]] ZP_FORCEINLINE const_iterator begin() const;

        [[nodiscard]] ZP_FORCEINLINE const_iterator end() const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray<T> split( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray<T> split( zp_size_t index, zp_size_t length ) const;

        [[nodiscard]] ZP_FORCEINLINE ReadonlyMemoryArray<T> asReadonly() const;

    private:
        value_type m_ptr[Size];
    };
}

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
    zp_size_t Vector<T, Allocator>::sizeAtomic() const
    {
        return Atomic::AddSizeT( m_size, 0 );
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isEmpty() const
    {
        return m_size == 0;
    }

    template<typename T, typename Allocator>
    zp_bool_t Vector<T, Allocator>::isEmptyAtomic() const
    {
        return Atomic::AddSizeT( m_size, 0 ) == 0;
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
    void Vector<T, Allocator>::pushBack( move_reference val )
    {
        if( m_size == m_capacity )
        {
            ensureCapacity( m_capacity * 2 );
        }
        m_data[ m_size++ ] = zp_move( val );
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackUnsafe( const_reference val )
    {
        m_data[ m_size++ ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackUnsafe( move_reference val )
    {
        m_data[ m_size++ ] = zp_move( val );
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackAtomic( const_reference val )
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_size ) - 1;
        m_data[ index ] = val;
    }

    template<typename T, typename Allocator>
    void Vector<T, Allocator>::pushBackAtomic( move_reference val )
    {
        const zp_size_t index = Atomic::IncrementSizeT( &m_size ) - 1;
        m_data[ index ] = zp_move( val );
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
    zp_size_t Vector<T, Allocator>::pushBackEmptyRange( zp_size_t count, zp_bool_t initialize )
    {
        const zp_size_t index = m_size;
        m_size += count;

        if( m_size >= m_capacity )
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
        const zp_size_t index = Atomic::AddSizeT( &m_size, count ) - count;

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
    template<typename Cmp>
    zp_size_t Vector<T, Allocator>::findIndexOf( Cmp cmp ) const
    {
        zp_size_t index = npos;

        if( m_size > 0 )
        {
            const_iterator b = m_data;
            const_iterator e = m_data + m_size;
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

        if( m_size > 0 )
        {
            const_iterator b = m_data;
            const_iterator e = m_data + m_size;
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

namespace zp
{
    template<typename T>
    zp_bool_t MemoryArray<T>::isEmpty() const
    {
        return !( ptr && length );
    }

    template<typename T>
    MemoryArray<T>::reference MemoryArray<T>::operator[]( zp_size_t index )
    {
        ZP_ASSERT( ptr && index < length );
        return ptr[ index ];
    }

    template<typename T>
    MemoryArray<T>::const_reference MemoryArray<T>::operator[]( zp_size_t index ) const
    {
        ZP_ASSERT( ptr && index < length );
        return ptr[ index ];
    }

    template<typename T>
    MemoryArray<T>::iterator MemoryArray<T>::begin()
    {
        return ptr;
    }

    template<typename T>
    MemoryArray<T>::iterator MemoryArray<T>::end()
    {
        return ptr + length;
    }

    template<typename T>
    MemoryArray<T>::const_iterator MemoryArray<T>::begin() const
    {
        return ptr;
    }

    template<typename T>
    MemoryArray<T>::const_iterator MemoryArray<T>::end() const
    {
        return ptr + length;
    }

    template<typename T>
    MemoryArray<T> MemoryArray<T>::split( zp_size_t index ) const
    {
        ZP_ASSERT( ptr && index < length );
        return { .ptr = ptr + index, .length = length - index };
    }

    template<typename T>
    MemoryArray<T> MemoryArray<T>::split( zp_size_t index, zp_size_t count ) const
    {
        ZP_ASSERT( ptr && ( index + count ) < count );
        return { .ptr = ptr + index, .length = count };
    }
}

namespace zp
{
    template<typename T>
    MemoryList<T>::MemoryList( void* ptr, zp_size_t capacity )
        : m_ptr( static_cast<T*>( ptr ) )
        , m_length( 0 )
        , m_capacity( capacity )
    {
    }

    template<typename T>
    MemoryList<T>::MemoryList( Memory memory )
        : m_ptr( static_cast<T*>( memory.ptr ) )
        , m_length( 0 )
        , m_capacity( memory.size / sizeof( T ) )
    {
    }

    template<typename T>
    zp_bool_t MemoryList<T>::isEmpty() const
    {
        return m_ptr == nullptr || m_length == 0;
    }

    template<typename T>
    zp_size_t MemoryList<T>::length() const
    {
        return m_length;
    }

    template<typename T>
    zp_size_t MemoryList<T>::capacity() const
    {
        return m_capacity;
    }

    template<typename T>
    MemoryList<T>::reference MemoryList<T>::operator[]( zp_size_t index )
    {
        ZP_ASSERT( index < m_length );
        return *( m_ptr + index );
    }

    template<typename T>
    MemoryList<T>::const_reference MemoryList<T>::operator[]( zp_size_t index ) const
    {
        ZP_ASSERT( index < m_length );
        return *( m_ptr + index );
    }

    template<typename T>
    void MemoryList<T>::push_back( const T& value )
    {
        ZP_ASSERT( m_length < m_capacity );
        *( m_ptr + m_length ) = value;
        ++m_length;
    }

    template<typename T>
    void MemoryList<T>::push_back( T&& value )
    {
        ZP_ASSERT( m_length < m_capacity );
        *( m_ptr + m_length ) = value;
        ++m_length;
    }

    template<typename T>
    T& MemoryList<T>::push_back_empty()
    {
        ZP_ASSERT( m_length < m_capacity );
        T* value = m_ptr + m_length;
        *value = {};
        ++m_length;
        return *value;
    }

    template<typename T>
    T& MemoryList<T>::push_back_uninitialized()
    {
        ZP_ASSERT( m_length < m_capacity );
        T* value = m_ptr + m_length;
        ++m_length;
        return *value;
    }

    template<typename T>
    MemoryList<T>::iterator MemoryList<T>::begin()
    {
        return m_ptr;
    }

    template<typename T>
    MemoryList<T>::iterator MemoryList<T>::end()
    {
        return m_ptr + m_length;
    }

    template<typename T>
    MemoryList<T>::const_iterator MemoryList<T>::begin() const
    {
        return m_ptr;
    }

    template<typename T>
    MemoryList<T>::const_iterator MemoryList<T>::end() const
    {
        return m_ptr + m_length;
    }

    template<typename T>
    MemoryArray<T> MemoryList<T>::split( zp_size_t index ) const
    {
        ZP_ASSERT( index < m_length );
        return { .ptr = m_ptr + index, .length = m_length - index };
    }

    template<typename T>
    MemoryArray<T> MemoryList<T>::split( zp_size_t index, zp_size_t count ) const
    {
        ZP_ASSERT( ( count + index ) < m_length );
        return { .ptr = m_ptr + index, .length = count };
    }
}

namespace zp
{
    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::FixedArray()
        : m_ptr()
    {
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::FixedArray( T (& ptr)[Size] )
        : m_ptr( ptr )
    {
    }

    template<typename T, zp_size_t Size>
    template<typename ... Args>
    FixedArray<T, Size>::FixedArray( Args ... m )
        : m_ptr { zp_forward<T>( m )... }
    {
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::FixedArray( pointer ptr, zp_size_t length )
        : m_ptr()
    {
        for( zp_size_t i = 0, imax = zp_min( Size, length ); i < imax; ++i )
        {
            m_ptr[ i ] = ptr[ i ];
        }
    }

    template<typename T, zp_size_t Size>
    zp_size_t FixedArray<T, Size>::length() const
    {
        return Size;
    }

    template<typename T, zp_size_t Size>
    zp_bool_t FixedArray<T, Size>::empty() const
    {
        return Size == 0;
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::reference FixedArray<T, Size>::operator[]( zp_size_t index )
    {
        return m_ptr[ index ];
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::const_reference FixedArray<T, Size>::operator[]( zp_size_t index ) const
    {
        return m_ptr[ index ];
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::pointer FixedArray<T, Size>::data()
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::const_pointer FixedArray<T, Size>::data() const
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::iterator FixedArray<T, Size>::begin()
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::iterator FixedArray<T, Size>::end()
    {
        return m_ptr + Size;
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::const_iterator FixedArray<T, Size>::begin() const
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    FixedArray<T, Size>::const_iterator FixedArray<T, Size>::end() const
    {
        return m_ptr + Size;
    }

    template<typename T, zp_size_t Size>
    MemoryArray<T> FixedArray<T, Size>::split( zp_size_t index ) const
    {
        ZP_ASSERT( m_ptr && index < Size );
        return FixedArray<T, Size>( m_ptr + index, Size - index );
    }

    template<typename T, zp_size_t Size>
    MemoryArray<T> FixedArray<T, Size>::split( zp_size_t index, zp_size_t length ) const
    {
        ZP_ASSERT( m_ptr && ( index + length ) < Size );
        return FixedArray<T, Size>( m_ptr + index, length );
    }

    template<typename T, zp_size_t Size>
    ReadonlyMemoryArray<T> FixedArray<T, Size>::asReadonly() const
    {
        return ReadonlyMemoryArray<T>( m_ptr, Size );
    }
}

#endif //ZP_VECTOR_H
