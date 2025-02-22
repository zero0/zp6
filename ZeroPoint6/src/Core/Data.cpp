//
// Created by phosg on 7/30/2023.
//

#include "Core/Data.h"
#include "Core/Map.h"
#include "Core/Hash.h"

using namespace zp;

namespace
{
    enum
    {
        kDefaultDataStreamWriterBlockSize = 4 KB
    };
}

DataStreamWriter::DataStreamWriter( MemoryLabel memoryLabel )
    : m_buffer( nullptr )
    , m_position( 0 )
    , m_capacity( 0 )
    , m_blockSize( kDefaultDataStreamWriterBlockSize )
    , memoryLabel( memoryLabel )
{
}

DataStreamWriter::DataStreamWriter( MemoryLabel memoryLabel, zp_size_t blockSize )
    : m_buffer( nullptr )
    , m_position( 0 )
    , m_capacity( 0 )
    , m_blockSize( blockSize > 0 ? blockSize : kDefaultDataStreamWriterBlockSize )
    , memoryLabel( memoryLabel )
{
}

DataStreamWriter::DataStreamWriter( MemoryLabel memoryLabel, zp_uint8_t* ptr, zp_size_t capacity )
    : m_buffer( ptr )
    , m_position( 0 )
    , m_capacity( capacity )
    , m_blockSize( 0 )
    , memoryLabel( memoryLabel )
{
}

DataStreamWriter::~DataStreamWriter()
{
    ZP_SAFE_FREE( memoryLabel, m_buffer );
}

zp_size_t DataStreamWriter::position() const
{
    return m_position;
}

zp_size_t DataStreamWriter::capacity() const
{
    return m_capacity;
}

zp_size_t DataStreamWriter::seek( zp_ptrdiff_t offset, DataStreamSeekOrigin origin )
{
    const zp_size_t oldPosition = m_position;

    switch( origin )
    {
        case DataStreamSeekOrigin::Beginning:
            m_position = zp_max( zp_ptrdiff_t( 0 ), offset );
            break;
        case DataStreamSeekOrigin::Position:
            if( offset < 0 && -offset > m_position )
            {
                m_position = 0;
            }
            else
            {
                m_position += offset;
            }
            break;
        case DataStreamSeekOrigin::End:
            if( offset < 0 && -offset > m_capacity )
            {
                m_position = 0;
            }
            else
            {
                m_position = offset + m_capacity;
            }
            break;
        case DataStreamSeekOrigin::Exact:
            m_position = offset;
            break;
    }

    if( m_position > m_capacity )
    {
        ensureCapacity( m_position );
    }

    return oldPosition;
}

void DataStreamWriter::reset()
{
    m_position = 0;
}

Memory DataStreamWriter::memory() const
{
    return { .ptr = m_buffer, .size = m_position };
}

Memory DataStreamWriter::memory( zp_size_t offset, zp_size_t size ) const
{
    const zp_bool_t overflow = offset + size > m_capacity;
    if( overflow )
    {
        return {};
    }

    return {
        .ptr = m_buffer + offset,
        .size = size
    };
}

DataStreamWriter DataStreamWriter::slice( zp_size_t size )
{
    const zp_size_t oldPosition = m_position;
    m_position += size;

    if( m_position > m_capacity )
    {
        ensureCapacity( m_position );
    }

    return { memoryLabel, m_buffer + oldPosition, size };
}

zp_size_t DataStreamWriter::write( const void* ptr, zp_size_t size )
{
    const zp_size_t newPosition = m_position + size;
    if( newPosition > m_capacity )
    {
        ensureCapacity( newPosition );
    }

    zp_memcpy( m_buffer + m_position, m_capacity - m_position, ptr, size );

    const zp_size_t oldPosition = m_position;
    m_position = newPosition;
    return oldPosition;
}

zp_size_t DataStreamWriter::writeAt( const void* ptr, zp_size_t size, zp_ptrdiff_t offset, DataStreamSeekOrigin origin )
{
    const zp_size_t oldPosition = seek( offset, origin );

    const zp_size_t positionSize = m_position + size;
    if( positionSize > m_capacity )
    {
        ensureCapacity( positionSize );
    }

    zp_memcpy( m_buffer + m_position, m_capacity - m_position, ptr, size );

    m_position = oldPosition;
    return m_position;
}

zp_size_t DataStreamWriter::writeAlignment( zp_size_t alignment, zp_bool_t fillWithPadding, zp_uint8_t padding )
{
    ZP_ASSERT( zp_is_pow2( alignment ) );
    const zp_size_t alignedPosition = ZP_ALIGN_SIZE( m_position, alignment );
    if( alignedPosition > m_capacity )
    {
        ensureCapacity( alignedPosition );
    }

    const zp_size_t oldPosition = m_position;

    // fill padded data with provided data
    if( fillWithPadding )
    {
        for( ; m_position < alignedPosition; ++m_position )
        {
            m_buffer[ m_position ] = padding;
        }
    }
    else
    {
        // otherwise, fill with "random" data
        zp_hash64_t hash = zp_fnv64_1a( m_position );
        for( zp_size_t i = 0; m_position < alignedPosition; ++m_position, i += 8 )
        {
            if( i == 64 )
            {
                hash = zp_fnv64_1a( m_position, hash );
                i = 0;
            }

            m_buffer[ m_position ] = zp_uint8_t( 0xFFU & ( hash >> i ) );
        }
    }

    return oldPosition;
}

void DataStreamWriter::ensureCapacity( zp_size_t capacity )
{
    ZP_ASSERT_RETURN( m_blockSize > 0 );

    zp_size_t newCapacity = m_blockSize;
    while( newCapacity < capacity )
    {
        newCapacity += m_blockSize;
    }

    zp_uint8_t* newBuffer = ZP_MALLOC_T_ARRAY( memoryLabel, zp_uint8_t, newCapacity );
    if( m_buffer )
    {
        zp_memcpy( newBuffer, newCapacity, m_buffer, m_position );
        ZP_FREE( memoryLabel, m_buffer );
    }

    m_buffer = newBuffer;
    m_capacity = newCapacity;
}

//
//
//

DataStreamReader::DataStreamReader( Memory memory )
    : m_memory( memory )
    , m_position( 0 )
{
}

zp_size_t DataStreamReader::position() const
{
    return m_position;
}

zp_bool_t DataStreamReader::end() const
{
    return m_position >= m_memory.size;
}

zp_size_t DataStreamReader::seek( zp_ptrdiff_t offset, DataStreamSeekOrigin origin )
{
    const zp_size_t oldPosition = m_position;

    switch( origin )
    {
        case DataStreamSeekOrigin::Beginning:
            m_position = zp_max( zp_ptrdiff_t( 0 ), offset );
            break;
        case DataStreamSeekOrigin::Position:
            if( offset < 0 && -offset > m_position )
            {
                m_position = 0;
            }
            else
            {
                m_position += offset;
            }
            break;
        case DataStreamSeekOrigin::End:
            if( offset < 0 && -offset > m_memory.size )
            {
                m_position = 0;
            }
            else
            {
                m_position = offset + m_memory.size;
            }
            break;
        case DataStreamSeekOrigin::Exact:
            m_position = offset;
            break;
    }

    return oldPosition;
}

void DataStreamReader::reset()
{
    m_position = 0;
}

Memory DataStreamReader::memory() const
{
    return m_memory;
}

Memory DataStreamReader::memory( zp_size_t offset, zp_size_t size ) const
{
    return { .ptr = ZP_OFFSET_PTR( m_memory.ptr, offset ), .size = size };
}

DataStreamReader DataStreamReader::slice( zp_size_t size )
{
    return DataStreamReader( { .ptr = ZP_OFFSET_PTR( m_memory.ptr, m_position ), .size = size } );
}

zp_size_t DataStreamReader::read( void* ptr, zp_size_t size )
{
    const zp_size_t oldPosition = m_position;

    const zp_size_t newPosition = m_position + size;
    if( newPosition < m_memory.size )
    {
        zp_memcpy( ptr, size, ZP_OFFSET_PTR( m_memory.ptr, m_position ), m_memory.size - m_position );
    }
    m_position = newPosition;

    return oldPosition;
}

zp_size_t DataStreamReader::readReverse( void* ptr, zp_size_t size )
{
    const zp_size_t oldPosition = m_position;

    if( m_position >= size )
    {
        const zp_size_t newPosition = m_position - size;
        zp_memcpy( ptr, size, ZP_OFFSET_PTR( m_memory.ptr, newPosition ), m_memory.size - newPosition );
        m_position = newPosition;
    }
    else
    {
        m_position = 0;
    }

    return oldPosition;
}

Memory DataStreamReader::readMemory( zp_size_t size )
{
    Memory memory {};

    const zp_size_t newPosition = m_position + size;
    if( newPosition < m_memory.size )
    {
        memory = { .ptr = ZP_OFFSET_PTR( m_memory.ptr, m_position ), .size = size };
        m_position = newPosition;
    }

    return memory;
}

void* DataStreamReader::readPtr( zp_size_t size )
{
    void* ptr {};

    const zp_size_t newPosition = m_position + size;
    if( newPosition < m_memory.size )
    {
        ptr = ZP_OFFSET_PTR( m_memory.ptr, m_position );
        m_position = newPosition;
    }

    return ptr;
}

void* DataStreamReader::readPtrArray( zp_size_t size, zp_size_t length )
{
    void* ptr {};

    const zp_size_t newPosition = m_position + ( size * length );
    if( newPosition < m_memory.size )
    {
        ptr = ZP_OFFSET_PTR( m_memory.ptr, m_position );
        m_position = newPosition;
    }

    return ptr;
}

zp_size_t DataStreamReader::readAlignment( zp_size_t alignment )
{
    ZP_ASSERT( zp_is_pow2( alignment ) );

    const zp_size_t oldPosition = m_position;

    const zp_size_t alignedPosition = ZP_ALIGN_SIZE( m_position, alignment );
    m_position = alignedPosition;

    return oldPosition;
}

//
//
//

typedef Vector<DataBuilderElement> DataBuilderElementArray;
typedef Map<AllocString, DataBuilderElement> DataBuilderElementObject;

DataBuilderElement::DataBuilderElement( MemoryLabel memoryLabel )
    : m_data()
    , m_dataType( ZP_DATA_BUILDER_ELEMENT_TYPE_NULL )
    , memoryLabel( memoryLabel )
{
}

DataBuilderElement::DataBuilderElement( MemoryLabel memoryLabel, DataBuilderElementType dataType )
    : m_data()
    , m_dataType( dataType )
    , memoryLabel( memoryLabel )
{
}

DataBuilderElement::DataBuilderElement( MemoryLabel memoryLabel, const char* str )
    : m_data()
    , m_dataType( ZP_DATA_BUILDER_ELEMENT_TYPE_NULL )
    , memoryLabel( memoryLabel )
{
    set( str );
}

DataBuilderElement::DataBuilderElement( MemoryLabel memoryLabel, Memory memory )
    : m_data()
    , m_dataType( ZP_DATA_BUILDER_ELEMENT_TYPE_NULL )
    , memoryLabel( memoryLabel )
{
    set( memory );
}

DataBuilderElement::DataBuilderElement( const DataBuilderElement& other )
    : m_data()
    , m_dataType( other.m_dataType )
    , memoryLabel( other.memoryLabel )
{
    zp_memcpy( m_data, ZP_ARRAY_SIZE( m_data ), other.m_data, ZP_ARRAY_SIZE( other.m_data ) );
}

DataBuilderElement::DataBuilderElement( DataBuilderElement&& other )
    : m_data()
    , m_dataType( other.m_dataType )
    , memoryLabel( other.memoryLabel )
{
    zp_memcpy( m_data, ZP_ARRAY_SIZE( m_data ), other.m_data, ZP_ARRAY_SIZE( other.m_data ) );

    other.ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_NULL );
}

DataBuilderElement::~DataBuilderElement()
{
    destroy();
}

void DataBuilderElement::set( zp_bool_t value )
{
    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_BOOLEAN );
    asRef<zp_bool_t>() = value;
}

void DataBuilderElement::set( zp_uint32_t value )
{
    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_UINT32 );
    asRef<zp_uint32_t>() = value;
}

void DataBuilderElement::set( zp_float32_t value )
{
    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_FLOAT32 );
    asRef<zp_float32_t>() = value;
}

void DataBuilderElement::set( const char* value )
{
    String strValue = String::As( value );

    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_STRING );
    asRef<AllocString>() = AllocString( memoryLabel, strValue.str(), strValue.length() );
}

void DataBuilderElement::set( Memory value )
{
    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_DATA );
    asRef<AllocMemory>() = AllocMemory( memoryLabel, value );
}

DataBuilderElement& DataBuilderElement::operator[]( zp_size_t index )
{
    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_ARRAY );
    DataBuilderElementArray& array = asRef<DataBuilderElementArray>();

    while( index >= array.length() )
    {
        array.pushBackEmptyRange( 1 );
    }

    return array[ index ];
}

DataBuilderElement& DataBuilderElement::operator[]( const char* key )
{
    ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_OBJECT );
    DataBuilderElementObject& obj = asRef<DataBuilderElementObject>();

    return obj.get( AllocString( memoryLabel, key ) );
}

DataBuilderElement& DataBuilderElement::operator=( const DataBuilderElement& other )
{
    if( &other != this )
    {
        ensureDataType( other.m_dataType );

        zp_memcpy( m_data, ZP_ARRAY_SIZE( m_data ), other.m_data, ZP_ARRAY_SIZE( other.m_data ) );
    }

    return *this;
}

DataBuilderElement& DataBuilderElement::operator=( DataBuilderElement&& other )
{
    if( &other != this )
    {
        ensureDataType( other.m_dataType );

        zp_memcpy( m_data, ZP_ARRAY_SIZE( m_data ), other.m_data, ZP_ARRAY_SIZE( other.m_data ) );

        other.ensureDataType( ZP_DATA_BUILDER_ELEMENT_TYPE_NULL );
    }

    return *this;
}

void DataBuilderElement::ensureDataType( DataBuilderElementType dataType )
{
    if( m_dataType != dataType )
    {
        destroy();

        m_dataType = dataType;
    }
}

void DataBuilderElement::destroy()
{
    switch( m_dataType )
    {
        case ZP_DATA_BUILDER_ELEMENT_TYPE_STRING:
            asRef<AllocString>() = {};
            break;

        case ZP_DATA_BUILDER_ELEMENT_TYPE_DATA:
            asRef<AllocMemory>() = {};
            break;

        case ZP_DATA_BUILDER_ELEMENT_TYPE_ARRAY:
            asRef<DataBuilderElementArray>().destroy();
            break;

        case ZP_DATA_BUILDER_ELEMENT_TYPE_OBJECT:
            asRef<DataBuilderElementObject>().destroy();
            break;

        default:
            break;
    }

    zp_zero_memory_array( m_data );
    m_dataType = ZP_DATA_BUILDER_ELEMENT_TYPE_NULL;
}

//
//
//

DataBuilder::DataBuilder( MemoryLabel memoryLabel )
    : m_elements( 4, memoryLabel )
    , memoryLabel( memoryLabel )
{
}

DataBuilderCompilerResult DataBuilder::compile( DataBuilderCompilerOptions options, DataStreamWriter& outCompiledData ) const
{

    return DataBuilderCompilerResult::Success;
}

//
//
//

namespace
{
    struct ArchiveHeader
    {
        zp_uint32_t id;
        zp_uint32_t dataVersion;
        zp_uint32_t version;
        zp_uint32_t flags;
    };

    struct ArchiveBlockHeader
    {
        zp_uint32_t type;
        zp_uint32_t headerSize; // size of un compressed header defined by block
        zp_uint64_t dataSize; // actual size, 0 == info.length, 0 != decompressed size
    };

    struct ArchiveBlockInfo
    {
        zp_uint64_t offset; // from header to block header
        zp_uint64_t length; // in data (without padding)
    };

    struct ArchiveBlockHash
    {
        zp_hash128_t hash; // hash including block header
    };

    struct ArchiveBlockID
    {
        zp_hash64_t id;
    };

    struct ArchiveFooter
    {
        zp_uint32_t id;
        zp_uint32_t flags;
        zp_uint64_t blockCount;
        zp_hash128_t hash; // hash of entire file
    };

    const zp_size_t kBlockAlignment = 16;
}

ArchiveBuilder::ArchiveBuilder( MemoryLabel memoryLabel )
    : m_blocks( 16, memoryLabel )
    , memoryLabel( memoryLabel )
{

}

ArchiveBuilder::~ArchiveBuilder()
{

}

ArchiveBuilderResult ArchiveBuilder::loadArchive( Memory archiveMemory )
{
    DataStreamReader reader( archiveMemory );

    ArchiveHeader header {};
    reader.read( header );

    reader.seek( 0, DataStreamSeekOrigin::End );

    ArchiveFooter footer {};
    reader.readReverse( footer );

    const zp_size_t blockCount = footer.blockCount;

    MemoryArray<ArchiveBlockID> blockIds {};
    reader.readReverse( blockIds, blockCount );

    MemoryArray<ArchiveBlockHash> blockHashes {};
    reader.readReverse( blockHashes, blockCount );

    MemoryArray<ArchiveBlockInfo> blockInfos {};
    reader.readReverse( blockInfos, blockCount );

    // read in blocks
    for( zp_size_t i = 0; i < blockCount; ++i )
    {
        reader.seek( zp_ptrdiff_t( blockInfos[ i ].offset ), DataStreamSeekOrigin::Exact );

        ArchiveBlockHeader blockHeader {};
        reader.read( blockHeader );

        ArchiveBuilderBlock block {
            .id  = blockIds[ i ].id,
            .type = 0,
            .flags = ZP_ARCHIVE_BUILDER_BLOCK_FLAG_NONE,
        };

        if( blockHeader.headerSize > 0 )
        {
            block.header = reader.readMemory( blockHeader.headerSize );

            reader.readAlignment( kBlockAlignment );
        }

        if( blockHeader.dataSize > 0 )
        {
            block.data = reader.readMemory( blockHeader.dataSize );

            reader.readAlignment( kBlockAlignment );
        }

        m_blocks.pushBack( block );
    }

    return ArchiveBuilderResult::Success;
}

zp_hash64_t ArchiveBuilder::addBlock( String name, Memory data )
{
    const zp_hash64_t id = zp_fnv64_1a( name.c_str(), name.length() );
    const zp_size_t index = m_blocks.findIndexOf( [ &id ]( const ArchiveBuilderBlock& v ) -> zp_bool_t
    {
        return v.id == id;
    } );

    if( index == zp::npos )
    {
        m_blocks.pushBack( {
            .id = id,
            .flags = ZP_ARCHIVE_BUILDER_BLOCK_FLAG_NONE,
            .header = {},
            .data = data,
        } );
    }
    else
    {
        m_blocks[ index ] = {
            .id = id,
            .flags = ZP_ARCHIVE_BUILDER_BLOCK_FLAG_NONE,
            .header = {},
            .data = data,
        };
    }

    return id;
}

zp_hash64_t ArchiveBuilder::addBlock( String name, Memory header, Memory data )
{
    const zp_hash64_t id = zp_fnv64_1a( name.c_str(), name.length() );
    const zp_size_t index = m_blocks.findIndexOf( [ &id ]( const ArchiveBuilderBlock& v ) -> zp_bool_t
    {
        return v.id == id;
    } );

    if( index == zp::npos )
    {
        m_blocks.pushBack( {
            .id = id,
            .flags = ZP_ARCHIVE_BUILDER_BLOCK_FLAG_NONE,
            .header = header,
            .data = data,
        } );
    }
    else
    {
        m_blocks[ index ] = {
            .id = id,
            .flags = ZP_ARCHIVE_BUILDER_BLOCK_FLAG_NONE,
            .header = header,
            .data = data,
        };
    }

    return id;
}

void ArchiveBuilder::clearBlock( zp_hash64_t id )
{
    zp_size_t index = m_blocks.findIndexOf( [ &id ]( const ArchiveBuilderBlock& v ) -> zp_bool_t
    {
        return v.id == id;
    } );

    if( index != zp::npos )
    {
        ArchiveBuilderBlock& block = m_blocks[ index ];
        block.flags |= ZP_ARCHIVE_BUILDER_BLOCK_FLAG_CLEAR;
    }
}

void ArchiveBuilder::removeBlock( zp_hash64_t id )
{
    zp_size_t index = m_blocks.findIndexOf( [ &id ]( const ArchiveBuilderBlock& v ) -> zp_bool_t
    {
        return v.id == id;
    } );

    if( index != zp::npos )
    {
        ArchiveBuilderBlock& block = m_blocks[ index ];
        block.flags |= ZP_ARCHIVE_BUILDER_BLOCK_FLAG_REMOVE;
    }
}

void ArchiveBuilder::getBlockIDs( Vector<zp_hash64_t>& outBlockIDs )
{
    outBlockIDs.clear();
    for( const ArchiveBuilderBlock& block : m_blocks )
    {
        outBlockIDs.pushBack( block.id );
    }
}

ArchiveBuilderResult ArchiveBuilder::compile( ArchiveBuilderCompilerOptions options, DataStreamWriter& outCompiledData ) const
{
    const zp_size_t blockCount = m_blocks.length();

    // sort data blocks by id
    struct BlockSortKey
    {
        zp_hash64_t id;
        zp_size_t index;
    };
    Vector<BlockSortKey> blockSort( blockCount, memoryLabel );

    for( zp_size_t i = 0; i < blockCount; ++i )
    {
        const ArchiveBuilderBlock& block = m_blocks[ i ];

        // skip sorting blocks that are removed
        if( ( block.flags & ZP_ARCHIVE_BUILDER_BLOCK_FLAG_REMOVE ) != ZP_ARCHIVE_BUILDER_BLOCK_FLAG_REMOVE )
        {
            blockSort.pushBack( { .id = block.id, .index = i } );
        }
    }

    const zp_size_t sortedBlockCount = blockSort.length();

    blockSort.sort( []( const BlockSortKey& x, const BlockSortKey& y )
    {
        return zp_cmp( x.id, y.id );
    } );

    // write archive header
    outCompiledData.reset();

    outCompiledData.write<ArchiveHeader>( {
        .id = zp_make_cc4( "ZARH" ),
        .dataVersion = 0,
        .version = 0,
        .flags = 0
    } );

    // write each block in sorted order
    struct BlockOffsetSize
    {
        zp_size_t offset;
        zp_size_t size;     // block size without alignment
    };
    Map<zp_hash64_t, BlockOffsetSize> blockIdToOffset( memoryLabel, sortedBlockCount );

    for( zp_size_t i = 0; i < sortedBlockCount; ++i )
    {
        const BlockSortKey& blockSortKey = blockSort[ i ];
        const ArchiveBuilderBlock& block = m_blocks[ blockSortKey.index ];
        zp_size_t offset;

        // write empty block
        if( ( block.flags & ZP_ARCHIVE_BUILDER_BLOCK_FLAG_CLEAR ) == ZP_ARCHIVE_BUILDER_BLOCK_FLAG_CLEAR )
        {
            offset = outCompiledData.write<ArchiveBlockHeader>( {
                .type = block.type,
                .headerSize = 0,
                .dataSize = 0
            } );
        }
        else
        {
            offset = outCompiledData.write<ArchiveBlockHeader>( {
                .type = block.type,
                .headerSize = block.header.ptr && block.header.size ? zp_uint32_t( block.header.size ) : 0,
                .dataSize = block.data.ptr && block.data.size ? zp_uint32_t( block.data.size ) : 0,
            } );
// TODO: compress?
            // write header if provided
            if( block.header.ptr && block.header.size )
            {
                outCompiledData.write( block.header.ptr, block.header.size );
            }

            // write data if provided
            if( block.data.ptr && block.data.size )
            {
                outCompiledData.write( block.data.ptr, block.data.size );
            }
        }

        // store offset
        blockIdToOffset.set( blockSortKey.id, { .offset = offset, .size = outCompiledData.position() - offset } );

        // align blocks
        const zp_bool_t fillPadding = ( ( options & ZP_ARCHIVE_BUILDER_COMPILER_OPTION_PAD_ZEROS ) == ZP_ARCHIVE_BUILDER_COMPILER_OPTION_PAD_ZEROS );
        outCompiledData.writeAlignment( kBlockAlignment, fillPadding, 0xCD );
    }
    ZP_ASSERT( outCompiledData.position() == ZP_ALIGN_SIZE( outCompiledData.position(), kBlockAlignment ) );

    // write info data
    for( zp_size_t i = 0; i < sortedBlockCount; ++i )
    {
        const BlockSortKey& blockSortKey = blockSort[ i ];
        const ArchiveBuilderBlock& block = m_blocks[ blockSortKey.index ];
        const BlockOffsetSize& offsetSize = blockIdToOffset[ blockSortKey.id ];

        outCompiledData.write<ArchiveBlockInfo>( {
            .offset = offsetSize.offset,
            .length = offsetSize.size,
        } );
    }

    // write hash data
    for( zp_size_t i = 0; i < sortedBlockCount; ++i )
    {
        const BlockSortKey& blockSortKey = blockSort[ i ];
        const BlockOffsetSize& offsetSize = blockIdToOffset[ blockSortKey.id ];

        // hash writen data, including alignment padding
        const Memory mem = outCompiledData.memory( offsetSize.offset, offsetSize.size );

        const zp_hash128_t blockHash = zp_fnv128_1a( mem );
        outCompiledData.write<ArchiveBlockHash>( { .hash = blockHash } );
    }

    // write sorted id data
    for( zp_size_t i = 0; i < sortedBlockCount; ++i )
    {
        const BlockSortKey& blockSortKey = blockSort[ i ];

        outCompiledData.write<ArchiveBlockID>( { .id = blockSortKey.id } );
    }

    // hash archive
    const Memory archiveMemory = outCompiledData.memory();
    const zp_hash128_t archiveHash = zp_fnv128_1a( archiveMemory );

    // write footer
    outCompiledData.write<ArchiveFooter>( {
        .id = zp_make_cc4( "ZARF" ),
        .flags = 0,
        .blockCount = sortedBlockCount,
        .hash = archiveHash
    } );

    return ArchiveBuilderResult::Success;
}

void test()
{
    DataBuilder builder( 0 );


    DataStreamWriter compiledMemory( 0 );
    builder.compile( ZP_DATA_BUILDER_COMPILER_OPTION_NONE, compiledMemory );
}
