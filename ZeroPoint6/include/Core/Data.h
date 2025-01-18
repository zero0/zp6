//
// Created by phosg on 7/28/2023.
//

#ifndef ZP_DATA_H
#define ZP_DATA_H

#include "Core/Allocator.h"
#include "Core/Vector.h"
#include "Core/Memory.h"
#include "Core/String.h"

namespace zp
{
    enum class DataStreamSeekOrigin
    {
        Beginning,
        Position,
        End,
        Exact,
    };

    class DataStreamWriter
    {
    public:
        explicit DataStreamWriter( MemoryLabel memoryLabel );

        DataStreamWriter( MemoryLabel memoryLabel, zp_size_t blockSize );

        ~DataStreamWriter();

        [[nodiscard]] zp_size_t position() const;

        [[nodiscard]] zp_size_t capacity() const;

        zp_size_t seek( zp_ptrdiff_t offset, DataStreamSeekOrigin origin );

        void reset();

        [[nodiscard]] Memory memory() const;

        [[nodiscard]] Memory memory( zp_size_t offset, zp_size_t size ) const;

        DataStreamWriter slice( zp_size_t size );

        zp_size_t write( const void* ptr, zp_size_t size );

        zp_size_t writeAt( const void* ptr, zp_size_t size, zp_ptrdiff_t offset, DataStreamSeekOrigin origin );

        template<typename T, zp_size_t Size>
        zp_size_t write( T (& arr)[Size] )
        {
            return write( arr, sizeof( T ) * Size );
        }

        template<typename T>
        zp_size_t write( const T& value )
        {
            return write( &value, sizeof( T ) );
        }

        template<typename T>
        zp_size_t write( T&& value )
        {
            return write( &value, sizeof( T ) );
        }

        template<typename T, zp_size_t Size>
        zp_size_t writeAt( T (& arr)[Size], zp_ptrdiff_t offset, DataStreamSeekOrigin origin )
        {
            return writeAt( arr, sizeof( T ) * Size, offset, origin );
        }

        template<typename T>
        zp_size_t writeAt( const T& value, zp_ptrdiff_t offset, DataStreamSeekOrigin origin )
        {
            return writeAt( &value, sizeof( T ), offset, origin );
        }

        template<typename T>
        zp_size_t writeAt( T&& value, zp_ptrdiff_t offset, DataStreamSeekOrigin origin )
        {
            return writeAt( &value, sizeof( T ), offset, origin );
        }

        zp_size_t writeAlignment( zp_size_t alignment, zp_bool_t fillWithPadding = true, zp_uint8_t padding = 0 );

    private:
        DataStreamWriter( MemoryLabel memoryLabel, zp_uint8_t* ptr, zp_size_t capacity );

        void ensureCapacity( zp_size_t capacity );

        zp_uint8_t* m_buffer;
        zp_size_t m_position;
        zp_size_t m_capacity;
        zp_size_t m_blockSize;

    public:
        const MemoryLabel memoryLabel;
    };

    class DataStreamReader
    {
    public:
        DataStreamReader() = delete;

        explicit DataStreamReader( Memory memory );

        ~DataStreamReader() = default;

        [[nodiscard]] zp_size_t position() const;

        zp_size_t seek( zp_ptrdiff_t offset, DataStreamSeekOrigin origin );

        void reset();

        [[nodiscard]] Memory memory() const;

        [[nodiscard]] Memory memory( zp_size_t offset, zp_size_t size ) const;

        DataStreamReader slice( zp_size_t size );

        zp_size_t read( void* ptr, zp_size_t size );

        zp_size_t readReverse( void* ptr, zp_size_t size );

        Memory readMemory( zp_size_t size );

        template<typename T>
        zp_size_t read( T& value )
        {
            return read( &value, sizeof( T ) );
        }

        template<typename T>
        zp_size_t readReverse( T& value )
        {
            return readReverse( &value, sizeof( T ) );
        }

        template<typename T>
        zp_size_t readReverse( MemoryArray<T>& value, zp_size_t count )
        {
            return readReverse( value.data(), sizeof( T ) * count );
        }

        zp_size_t readAlignment( zp_size_t alignment );

    private:
        Memory m_memory;
        zp_size_t m_position;
    };
}

constexpr zp_uint32_t zp_make_cc4( char c0, char c1, char c2, char c3 )
{
    zp_uint32_t cc = 0;
    cc |= c0 << 24;
    cc |= c1 << 16;
    cc |= c2 << 8;
    cc |= c3 << 0;
    return cc;
}

constexpr zp_uint32_t zp_make_cc4( const char* str )
{
    const zp_size_t len = zp_strlen( str );

    zp_uint32_t cc = 0;
    for( zp_size_t i = 0; i < 4 && i < len; ++i )
    {
        cc |= str[ i ] << ( 32 - ( ( i + 1 ) * 8 ) );
        //cc |= str[i] << ( i * 8 );
    }
    return cc;
}

constexpr zp_uint64_t zp_make_cc8( const char* str )
{
    const zp_size_t len = zp_strlen( str );

    zp_uint64_t cc = 0;
    for( zp_size_t i = 0; i < 8 && i < len; ++i )
    {
        cc |= str[ i ] << ( 64 - ( ( i + 1 ) * 8 ) );
    }
    return cc;
}

namespace zp
{
    enum DataBuilderCompilerOptions
    {
        ZP_DATA_BUILDER_COMPILER_OPTION_NONE = 0,

        ZP_DATA_BUILDER_COMPILER_OPTION_BLOCK_COMPRESS = 1 << 0,
        ZP_DATA_BUILDER_COMPILER_OPTION_FORCE_FLOAT16 = 1 << 1,
        ZP_DATA_BUILDER_COMPILER_OPTION_REMOVE_EMPTY = 1 << 2,

        ZP_DATA_BUILDER_COMPILER_OPTION_ALL = ~0,
    };

    enum class DataBuilderCompilerResult
    {
        Success = 0,
        Failed = -1,
    };

    enum DataType
    {
        ZP_DATA_TYPE_NULL,

        ZP_DATA_TYPE_BOOLEAN_FALSE,
        ZP_DATA_TYPE_BOOLEAN_TRUE,

        ZP_DATA_TYPE_UINT_ZERO,
        ZP_DATA_TYPE_UINT8,
        ZP_DATA_TYPE_UINT16,
        ZP_DATA_TYPE_UINT32,
        ZP_DATA_TYPE_UINT64,

        ZP_DATA_TYPE_INT_ZERO,
        ZP_DATA_TYPE_INT8,
        ZP_DATA_TYPE_INT16,
        ZP_DATA_TYPE_INT32,
        ZP_DATA_TYPE_INT64,

        ZP_DATA_TYPE_FLOAT_ZERO,
        ZP_DATA_TYPE_FLOAT16,
        ZP_DATA_TYPE_FLOAT32,
        ZP_DATA_TYPE_FLOAT64,

        ZP_DATA_TYPE_GUID_ZERO,
        ZP_DATA_TYPE_GUID128,

        ZP_DATA_TYPE_HASH_ZERO,
        ZP_DATA_TYPE_HASH64,
        ZP_DATA_TYPE_HASH128,

        ZP_DATA_TYPE_STRING_ZERO,
        ZP_DATA_TYPE_STRING,

        ZP_DATA_TYPE_DATA_ZERO,
        ZP_DATA_TYPE_DATA,

        ZP_DATA_TYPE_VECTOR_UINT_ZERO,
        ZP_DATA_TYPE_VECTOR_UINT8,
        ZP_DATA_TYPE_VECTOR_UINT16,
        ZP_DATA_TYPE_VECTOR_UINT32,
        ZP_DATA_TYPE_VECTOR_UINT64,

        ZP_DATA_TYPE_VECTOR_INT_ZERO,
        ZP_DATA_TYPE_VECTOR_INT8,
        ZP_DATA_TYPE_VECTOR_INT16,
        ZP_DATA_TYPE_VECTOR_INT32,
        ZP_DATA_TYPE_VECTOR_INT64,

        ZP_DATA_TYPE_VECTOR_FLOAT_ZERO,
        ZP_DATA_TYPE_VECTOR_FLOAT16,
        ZP_DATA_TYPE_VECTOR_FLOAT32,

        ZP_DATA_TYPE_MATRIX_FLOAT_ZERO,
        ZP_DATA_TYPE_MATRIX_FLOAT16,
        ZP_DATA_TYPE_MATRIX_FLOAT32,

        ZP_DATA_TYPE_ARRAY_ZERO,
        ZP_DATA_TYPE_ARRAY,

        ZP_DATA_TYPE_OBJECT_ZERO,
        ZP_DATA_TYPE_OBJECT,

        DataType_Count
    };

    class DataBuilder;

    enum DataBuilderElementType
    {
        ZP_DATA_BUILDER_ELEMENT_TYPE_NULL,

        ZP_DATA_BUILDER_ELEMENT_TYPE_BOOLEAN,

        ZP_DATA_BUILDER_ELEMENT_TYPE_UINT8,
        ZP_DATA_BUILDER_ELEMENT_TYPE_UINT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_UINT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_UINT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_INT8,
        ZP_DATA_BUILDER_ELEMENT_TYPE_INT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_INT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_INT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_FLOAT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_FLOAT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_FLOAT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_GUID128,

        ZP_DATA_BUILDER_ELEMENT_TYPE_HASH64,
        ZP_DATA_BUILDER_ELEMENT_TYPE_HASH128,

        ZP_DATA_BUILDER_ELEMENT_TYPE_STRING,

        ZP_DATA_BUILDER_ELEMENT_TYPE_DATA,

        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_UINT8,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_UINT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_UINT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_UINT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_INT8,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_INT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_INT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_INT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_FLOAT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_FLOAT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_VECTOR_FLOAT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_MATRIX_FLOAT16,
        ZP_DATA_BUILDER_ELEMENT_TYPE_MATRIX_FLOAT32,
        ZP_DATA_BUILDER_ELEMENT_TYPE_MATRIX_FLOAT64,

        ZP_DATA_BUILDER_ELEMENT_TYPE_ARRAY,

        ZP_DATA_BUILDER_ELEMENT_TYPE_OBJECT,

        DataBuilderElementType_Count
    };

    class DataBuilderElement
    {
    public:
        explicit DataBuilderElement( MemoryLabel memoryLabel );

        DataBuilderElement( MemoryLabel memoryLabel, DataBuilderElementType dataType );

        DataBuilderElement( MemoryLabel memoryLabel, const char* str );

        DataBuilderElement( MemoryLabel memoryLabel, Memory memory );

        DataBuilderElement( const DataBuilderElement& other );

        DataBuilderElement( DataBuilderElement&& other );

        ~DataBuilderElement();

        void set( zp_bool_t value );

        void set( zp_uint32_t value );

        void set( zp_float32_t value );

        void set( const char* value );

        void set( Memory value );

        DataBuilderElement& operator[]( zp_size_t index );

        DataBuilderElement& operator[]( const char* key );

        DataBuilderElement& operator=( const DataBuilderElement& other );

        DataBuilderElement& operator=( DataBuilderElement&& other );

    private:
        template<typename T>
        T* as()
        {
            ZP_STATIC_ASSERT( sizeof( T ) <= sizeof( m_data ) );
            T* ptr = reinterpret_cast<T*>( m_data );
            return ptr;
        }

        template<typename T>
        T*& asPtrRef()
        {
            ZP_STATIC_ASSERT( sizeof( T ) <= sizeof( m_data ) );
            T* ptr = reinterpret_cast<T*>( m_data );
            return ptr;
        }

        template<typename T>
        T& asRef()
        {
            ZP_STATIC_ASSERT( sizeof( T ) <= sizeof( m_data ) );
            T* ptr = reinterpret_cast<T*>( m_data );
            return *ptr;
        }

        void ensureDataType( DataBuilderElementType dataType );

        void destroy();

        zp_uint8_t m_data[128];

        DataBuilderElementType m_dataType;

    public:
        MemoryLabel memoryLabel;
    };

    class DataBuilder
    {
    public:
        explicit DataBuilder( MemoryLabel memoryLabel );


        DataBuilderCompilerResult compile( DataBuilderCompilerOptions options, DataStreamWriter& outCompiledData ) const;

    private:
        Vector<DataBuilderElement> m_elements;

    public:
        const MemoryLabel memoryLabel;
    };

    class DataBlockView
    {
    public:
        zp_size_t length() const;

        zp_bool_t empty() const;

        zp_size_t read( zp_bool_t& value ) const;

        zp_size_t read( zp_uint32_t& value ) const;

    private:
        void* m_data;
        zp_size_t m_length;
    };

    //
    //
    //

    enum ArchiveBuilderBlockFlag
    {
        ZP_ARCHIVE_BUILDER_BLOCK_FLAG_NONE = 0,
        ZP_ARCHIVE_BUILDER_BLOCK_FLAG_CLEAR = 1 << 0,
        ZP_ARCHIVE_BUILDER_BLOCK_FLAG_REMOVE = 1 << 1,
        ZP_ARCHIVE_BUILDER_BLOCK_FLAG_KEEP_UNCOMPRESSED = 1 << 2,
        ZP_ARCHIVE_BUILDER_BLOCK_FLAG_ALL = ~0u,
    };
    typedef zp_uint32_t ArchiveBuilderBlockFlags;

    struct ArchiveBuilderBlock
    {
        zp_hash64_t id;
        zp_uint32_t type;
        ArchiveBuilderBlockFlags flags;
        Memory header;
        Memory data;
        MemoryLabel memoryLabel;
    };

    enum class ArchiveBuilderResult
    {
        Success = 0,
        Failed = -1,
    };

    enum ArchiveBuilderCompilerOption
    {
        ZP_ARCHIVE_BUILDER_COMPILER_OPTION_NONE = 0,
        ZP_ARCHIVE_BUILDER_COMPILER_OPTION_PAD_ZEROS = 1 << 0,
        ZP_ARCHIVE_BUILDER_COMPILER_OPTION_ALL = ~0u,
    };
    typedef zp_uint32_t ArchiveBuilderCompilerOptions;

    class ArchiveBuilder
    {
    public:
        explicit ArchiveBuilder( MemoryLabel memoryLabel );

        ~ArchiveBuilder();

        // reload previous archive so it can be modified
        ArchiveBuilderResult loadArchive( Memory archiveMemory );

        // add or replace an existing block
        zp_hash64_t addBlock( String name, Memory data );

        // add or replace an existing block
        zp_hash64_t addBlock( String name, Memory header, Memory data );

        // keeps block, but removes data
        void clearBlock( zp_hash64_t id );

        // remove block fully
        void removeBlock( zp_hash64_t id );

        //
        void getBlockIDs( Vector<zp_hash64_t>& outBlockIDs );

        // compile archive to data
        ArchiveBuilderResult compile( ArchiveBuilderCompilerOptions options, DataStreamWriter& outCompiledData ) const;

    private:
        Vector<ArchiveBuilderBlock> m_blocks;

    public:
        const MemoryLabel memoryLabel;
    };

    class ArchiveMemoryView
    {
    public:
        zp_bool_t containsBlock( const char* blockName ) const;

        void readBlock();

    private:
        Memory m_archive;
    };

    class ArchiveFileView
    {
    public:
        zp_bool_t containsBlock( const char* blockName ) const;


    private:
        zp_handle_t m_archiveFile;
    };

    //
    //
    //

    class DataView
    {
    public:
        DataType type() const;

        zp_size_t length() const;

        zp_bool_t empty() const;

        zp_bool_t asBoolean() const;

        zp_int32_t asInt32() const;

        zp_float16_t asFloat16() const;

        zp_float32_t asFloat32() const;

        String asString() const;

        Memory asData() const;

        DataBlockView asBlock() const;

        DataView operator[]( zp_size_t index ) const;

        DataView operator[]( const char* name ) const;

        DataView operator[]( String name ) const;

    private:
        void* m_data;
        zp_size_t m_length;
    };
}

#endif //ZP_DATA_H
