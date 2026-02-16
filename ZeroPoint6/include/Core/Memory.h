//
// Created by phosg on 1/17/2025.
//

#ifndef ZP_MEMORY_H
#define ZP_MEMORY_H

#include "Core/Common.h"
#include "Core/Defines.h"
#include "Core/Math.h"
#include "Core/Types.h"

namespace zp
{
    struct ReadOnlyMemory
    {
        ZP_FORCEINLINE constexpr ReadOnlyMemory()
            : m_ptr( nullptr )
            , m_size( 0 )
        {
        }

        ZP_FORCEINLINE constexpr ReadOnlyMemory( const void* ptr, const zp_size_t size )
            : m_ptr( ptr )
            , m_size( size )
        {
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto ptr() const -> const void*
        {
            return m_ptr;
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto size() const -> zp_size_t
        {
            return m_size;
        }

        template<typename T>
        [[nodiscard]] ZP_FORCEINLINE constexpr auto as() const -> const T*
        {
            ZP_ASSERT( sizeof( T ) <= m_size );
            return static_cast<const T*>( m_ptr );
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto slice( const zp_size_t sliceSize ) const -> ReadOnlyMemory
        {
            ZP_ASSERT( sliceSize <= m_size );
            return { m_ptr, sliceSize };
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto slice( const zp_size_t offset, const zp_size_t sliceSize ) const -> ReadOnlyMemory
        {
            ZP_ASSERT( offset < m_size );
            ZP_ASSERT( sliceSize <= m_size - offset);
            return { static_cast<const zp_uint8_t*>( m_ptr ) + offset, sliceSize };
        }

    private:
        const void* m_ptr;
        zp_size_t m_size;
    };

    struct Memory
    {
        ZP_FORCEINLINE constexpr Memory()
            : m_ptr( nullptr )
            , m_size( 0 )
        {
        }

        ZP_FORCEINLINE constexpr Memory( void* ptr, const zp_size_t size )
            : m_ptr( ptr )
            , m_size( size )
        {
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto ptr() -> void*
        {
            return m_ptr;
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto ptr() const -> const void*
        {
            return m_ptr;
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto size() const -> zp_size_t
        {
            return m_size;
        }

        template<typename T>
        [[nodiscard]] ZP_FORCEINLINE constexpr auto as() -> T*
        {
            ZP_ASSERT( sizeof( T ) <= m_size );
            return static_cast<T*>( m_ptr );
        }

        template<typename T>
        [[nodiscard]] ZP_FORCEINLINE constexpr auto as() const -> const T*
        {
            ZP_ASSERT( sizeof( T ) <= m_size );
            return static_cast<const T*>( m_ptr );
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto slice( const zp_size_t sliceSize ) const -> Memory
        {
            return { m_ptr, sliceSize };
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto slice( const zp_size_t offset, const zp_size_t sliceSize ) const -> Memory
        {
            return { static_cast<zp_uint8_t*>( m_ptr ) + offset, sliceSize };
        }

        [[nodiscard]] ZP_FORCEINLINE constexpr auto asReadonly() const -> ReadOnlyMemory
        {
            return { m_ptr, m_size };
        }

    private:
        void* m_ptr;
        zp_size_t m_size;
    };

} // namespace zp

namespace zp
{
    template<typename T>
    struct MemoryArray
    {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;

        MemoryArray() = default;

        ~MemoryArray() = default;

        MemoryArray( pointer ptr, zp_size_t length );

        [[nodiscard]] ZP_FORCEINLINE zp_size_t length() const;

        [[nodiscard]] ZP_FORCEINLINE zp_size_t size() const;

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t empty() const;

        [[nodiscard]] ZP_FORCEINLINE reference operator[]( zp_size_t index );

        [[nodiscard]] ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE pointer data();

        [[nodiscard]] ZP_FORCEINLINE const_pointer data() const;

        [[nodiscard]] ZP_FORCEINLINE iterator begin();

        [[nodiscard]] ZP_FORCEINLINE iterator end();

        [[nodiscard]] ZP_FORCEINLINE const_iterator begin() const;

        [[nodiscard]] ZP_FORCEINLINE const_iterator end() const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray split( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE MemoryArray split( zp_size_t index, zp_size_t count ) const;

        [[nodiscard]] ZP_FORCEINLINE Memory memory();

        [[nodiscard]] ZP_FORCEINLINE Memory memory() const;

    private:
        pointer m_ptr;
        zp_size_t m_length;
    };
} // namespace zp

namespace zp
{
    template<typename T>
    class ReadonlyMemoryArray
    {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using const_pointer = const T*;
        using const_iterator = const T*;

        ReadonlyMemoryArray() = default;

        ~ReadonlyMemoryArray() = default;

        ReadonlyMemoryArray( const_pointer ptr, zp_size_t length );

        [[nodiscard]] ZP_FORCEINLINE zp_size_t length() const;

        [[nodiscard]] ZP_FORCEINLINE zp_bool_t empty() const;

        [[nodiscard]] ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE const_iterator begin() const;

        [[nodiscard]] ZP_FORCEINLINE const_iterator end() const;

        [[nodiscard]] ZP_FORCEINLINE ReadonlyMemoryArray split( zp_size_t index ) const;

        [[nodiscard]] ZP_FORCEINLINE ReadonlyMemoryArray split( zp_size_t index, zp_size_t length ) const;

    private:
        const_pointer m_ptr;
        zp_size_t m_length;
    };
} // namespace zp

namespace zp
{
    template<typename T, zp_size_t Size>
    class FixedArray
    {
    public:
        using value_type = T;
        using reference = T&;
        using move_type = T&&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = T*;
        using const_iterator = const T*;

        constexpr FixedArray();

        constexpr explicit FixedArray( value_type ( &ptr )[ Size ] );

        template<typename... Args>
        constexpr FixedArray( Args... args );

        constexpr FixedArray( pointer ptr, zp_size_t length );

        [[nodiscard]] constexpr ZP_FORCEINLINE zp_size_t length() const;

        [[nodiscard]] constexpr ZP_FORCEINLINE zp_bool_t empty() const;

        [[nodiscard]] constexpr ZP_FORCEINLINE reference operator[]( zp_size_t index );

        [[nodiscard]] constexpr ZP_FORCEINLINE const_reference operator[]( zp_size_t index ) const;

        [[nodiscard]] constexpr ZP_FORCEINLINE pointer data();

        [[nodiscard]] constexpr ZP_FORCEINLINE const_pointer data() const;

        [[nodiscard]] constexpr ZP_FORCEINLINE iterator begin();

        [[nodiscard]] constexpr ZP_FORCEINLINE iterator end();

        [[nodiscard]] constexpr ZP_FORCEINLINE const_iterator begin() const;

        [[nodiscard]] constexpr ZP_FORCEINLINE const_iterator end() const;

        [[nodiscard]] constexpr ZP_FORCEINLINE MemoryArray<T> split( zp_size_t index ) const;

        [[nodiscard]] constexpr ZP_FORCEINLINE MemoryArray<T> split( zp_size_t index, zp_size_t length ) const;

        [[nodiscard]] constexpr ZP_FORCEINLINE ReadonlyMemoryArray<T> asReadonly() const;

        [[nodiscard]] constexpr ZP_FORCEINLINE Memory asMemory();

        [[nodiscard]] constexpr ZP_FORCEINLINE ReadOnlyMemory asReadOnlyMemory() const;

    private:
        value_type m_ptr[ Size ];
    };

    template<typename T, typename... U>
    FixedArray( T, U... ) -> FixedArray<T, 1 + sizeof...( U )>;
} // namespace zp


//
// Impl
//

namespace zp
{
    template<typename T>
    MemoryArray<T>::MemoryArray( pointer ptr, zp_size_t length )
        : m_ptr( ptr ), m_length( length )
    {
    }

    template<typename T>
    zp_size_t MemoryArray<T>::length() const
    {
        return m_length;
    }

    template<typename T>
    zp_size_t MemoryArray<T>::size() const
    {
        return m_length * sizeof( T );
    }

    template<typename T>
    zp_bool_t MemoryArray<T>::empty() const
    {
        return !( m_ptr != nullptr && m_length != 0 );
    }

    template<typename T>
    MemoryArray<T>::reference MemoryArray<T>::operator[]( zp_size_t index )
    {
        ZP_ASSERT( m_ptr != nullptr && index < m_length );
        return m_ptr[ index ];
    }

    template<typename T>
    MemoryArray<T>::const_reference MemoryArray<T>::operator[]( zp_size_t index ) const
    {
        ZP_ASSERT( m_ptr != nullptr && index < m_length );
        return m_ptr[ index ];
    }

    template<typename T>
    MemoryArray<T>::pointer MemoryArray<T>::data()
    {
        return m_ptr;
    }

    template<typename T>
    MemoryArray<T>::const_pointer MemoryArray<T>::data() const
    {
        return m_ptr;
    }

    template<typename T>
    MemoryArray<T>::iterator MemoryArray<T>::begin()
    {
        return m_ptr;
    }

    template<typename T>
    MemoryArray<T>::iterator MemoryArray<T>::end()
    {
        return m_ptr + m_length;
    }

    template<typename T>
    MemoryArray<T>::const_iterator MemoryArray<T>::begin() const
    {
        return m_ptr;
    }

    template<typename T>
    MemoryArray<T>::const_iterator MemoryArray<T>::end() const
    {
        return m_ptr + m_length;
    }

    template<typename T>
    MemoryArray<T> MemoryArray<T>::split( zp_size_t index ) const
    {
        ZP_ASSERT( m_ptr != nullptr && index < m_length );
        return MemoryArray<T>( m_ptr + index, m_length - index );
    }

    template<typename T>
    MemoryArray<T> MemoryArray<T>::split( zp_size_t index, zp_size_t count ) const
    {
        ZP_ASSERT( m_ptr != nullptr && ( index + count ) < m_length );
        return MemoryArray<T>( m_ptr + index, count );
    }

    template<typename T>
    Memory MemoryArray<T>::memory()
    {
        return { m_ptr, m_length * sizeof( T ) };
    }

    template<typename T>
    Memory MemoryArray<T>::memory() const
    {
        return { m_ptr, m_length * sizeof( T ) };
    }
} // namespace zp

namespace zp
{
    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::FixedArray()
        : m_ptr()
    {
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::FixedArray( T ( &ptr )[ Size ] )
        : m_ptr( ptr )
    {
    }

    template<typename T, zp_size_t Size>
    template<typename... Args>
    constexpr FixedArray<T, Size>::FixedArray( Args... args )
        : m_ptr{ zp_forward<T>( args )... }
    {
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::FixedArray( pointer ptr, zp_size_t length )
        : m_ptr()
    {
        for( zp_size_t i = 0, imax = zp_min( Size, length ); i < imax; ++i )
        {
            m_ptr[ i ] = ptr[ i ];
        }
    }

    template<typename T, zp_size_t Size>
    constexpr zp_size_t FixedArray<T, Size>::length() const
    {
        return Size;
    }

    template<typename T, zp_size_t Size>
    constexpr zp_bool_t FixedArray<T, Size>::empty() const
    {
        return Size == 0;
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::reference FixedArray<T, Size>::operator[]( zp_size_t index )
    {
        ZP_ASSERT( index < Size );
        return m_ptr[ index ];
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::const_reference FixedArray<T, Size>::operator[]( zp_size_t index ) const
    {
        ZP_ASSERT( index < Size );
        return m_ptr[ index ];
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::pointer FixedArray<T, Size>::data()
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::const_pointer FixedArray<T, Size>::data() const
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::iterator FixedArray<T, Size>::begin()
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::iterator FixedArray<T, Size>::end()
    {
        return m_ptr + Size;
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::const_iterator FixedArray<T, Size>::begin() const
    {
        return m_ptr;
    }

    template<typename T, zp_size_t Size>
    constexpr FixedArray<T, Size>::const_iterator FixedArray<T, Size>::end() const
    {
        return m_ptr + Size;
    }

    template<typename T, zp_size_t Size>
    constexpr MemoryArray<T> FixedArray<T, Size>::split( zp_size_t index ) const
    {
        ZP_ASSERT( m_ptr && index < Size );
        return FixedArray<T, Size>( m_ptr + index, Size - index );
    }

    template<typename T, zp_size_t Size>
    constexpr MemoryArray<T> FixedArray<T, Size>::split( zp_size_t index, zp_size_t length ) const
    {
        ZP_ASSERT( m_ptr && ( index + length ) < Size );
        return FixedArray<T, Size>( m_ptr + index, length );
    }

    template<typename T, zp_size_t Size>
    constexpr ReadonlyMemoryArray<T> FixedArray<T, Size>::asReadonly() const
    {
        return ReadonlyMemoryArray<T>( m_ptr, Size );
    }

    template<typename T, zp_size_t Size>
    constexpr Memory FixedArray<T, Size>::asMemory()
    {
        return { m_ptr, Size * sizeof( T ) };
    }

    template<typename T, zp_size_t Size>
    constexpr ReadOnlyMemory FixedArray<T, Size>::asReadOnlyMemory() const
    {
        return { m_ptr, Size * sizeof( T ) };
    }
} // namespace zp


namespace zp
{
    template<typename T>
    ReadonlyMemoryArray<T>::ReadonlyMemoryArray( ReadonlyMemoryArray::const_pointer ptr, zp_size_t length )
        : m_ptr( ptr ), m_length( length )
    {
    }

    template<typename T>
    ReadonlyMemoryArray<T> ReadonlyMemoryArray<T>::split( zp_size_t index, zp_size_t length ) const
    {
        return ReadonlyMemoryArray<T>( m_ptr + index, length );
    }

    template<typename T>
    ReadonlyMemoryArray<T> ReadonlyMemoryArray<T>::split( zp_size_t index ) const
    {
        return ReadonlyMemoryArray<T>( m_ptr + index, m_length - index );
    }

    template<typename T>
    ReadonlyMemoryArray<T>::const_iterator ReadonlyMemoryArray<T>::end() const
    {
        return m_ptr + m_length;
    }

    template<typename T>
    ReadonlyMemoryArray<T>::const_iterator ReadonlyMemoryArray<T>::begin() const
    {
        return m_ptr;
    }

    template<typename T>
    ReadonlyMemoryArray<T>::const_reference ReadonlyMemoryArray<T>::operator[]( zp_size_t index ) const
    {
        ZP_ASSERT( index < m_length );
        return m_ptr[ index ];
    }

    template<typename T>
    zp_bool_t ReadonlyMemoryArray<T>::empty() const
    {
        return m_length == 0;
    }

    template<typename T>
    zp_size_t ReadonlyMemoryArray<T>::length() const
    {
        return m_length;
    }
} // namespace zp

ZP_FORCEINLINE void zp_memcpy( zp::Memory dst, const zp::Memory& src )
{
    zp_memcpy( dst.ptr(), dst.size(), src.ptr(), src.size() );
}

ZP_FORCEINLINE void zp_memcpy( zp::Memory dst, const zp::ReadOnlyMemory& src )
{
    zp_memcpy( dst.ptr(), dst.size(), src.ptr(), src.size() );
}

#endif // ZP_MEMORY_H
