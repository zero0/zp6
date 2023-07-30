//
// Created by phosg on 7/28/2023.
//

#ifndef ZP_DATA_H
#define ZP_DATA_H

#include "Core/Allocator.h"
#include "Core/String.h"

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

    class DataBuilder
    {
    public:
        explicit DataBuilder( MemoryLabel memoryLabel );

        ~DataBuilder();

        void beginRecodring();

        void endRecording();

        void pushObject( String name );

        void popObject();

        void pushArray( String name );

        void popArray();

        DataBuilder& operator[]( const char* name );

        DataBuilder& operator[]( zp_size_t index );

        DataBuilderCompilerResult compile( DataBuilderCompilerOptions options, Memory& outCompiledData ) const;

    private:

    public:
        const MemoryLabel memoryLabel;
    };

    enum DataType
    {
        ZP_DATA_TYPE_NULL,

        ZP_DATA_TYPE_BOOLEAN,

        ZP_DATA_TYPE_UINT8,
        ZP_DATA_TYPE_UINT16,
        ZP_DATA_TYPE_UINT32,
        ZP_DATA_TYPE_UINT64,

        ZP_DATA_TYPE_INT8,
        ZP_DATA_TYPE_INT16,
        ZP_DATA_TYPE_INT32,
        ZP_DATA_TYPE_INT64,

        ZP_DATA_TYPE_FLOAT16,
        ZP_DATA_TYPE_FLOAT32,

        ZP_DATA_TYPE_GUID128,

        ZP_DATA_TYPE_HASH64,
        ZP_DATA_TYPE_HASH128,

        ZP_DATA_TYPE_STRING,
        ZP_DATA_TYPE_DATA,

        ZP_DATA_TYPE_VECTOR,
        ZP_DATA_TYPE_MATRIX,

        ZP_DATA_TYPE_ARRAY,
        ZP_DATA_TYPE_OBJECT,
    };

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

        DataView operator[]( zp_size_t index ) const;

        DataView operator[]( const char* name ) const;

        DataView operator[]( String name ) const;

    private:
        void* m_data;
        zp_size_t m_length;
    };
}

#endif //ZP_DATA_H
