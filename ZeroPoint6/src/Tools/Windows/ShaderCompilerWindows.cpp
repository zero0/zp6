//
// Created by phosg on 7/20/2023.
//

#include <Windows.h>

#include "Core/Allocator.h"
#include "Core/Data.h"
#include "Core/Job.h"
#include "Core/Vector.h"
#include "Core/String.h"
#include "Platform/Platform.h"

#include "Tools/ShaderCompiler.h"

#pragma comment(lib, "dxcompiler.lib")

constexpr int char_to_int4( int c )
{
    switch( c )
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return c - '0';

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return 10 + ( c - 'a' );

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return 10 + ( c - 'A' );

        default:
            return 0;
    }
}

constexpr int spec_to_int8( const char* spec )
{
    int r = 0;
    r |= char_to_int4( spec[ 0 ] ) << 4;
    r |= char_to_int4( spec[ 1 ] ) << 0;
    return r;
}

constexpr int spec_to_int32( const char* spec )
{
    int r = 0;
    r |= spec_to_int8( spec + 0 ) << 24;
    r |= spec_to_int8( spec + 2 ) << 16;
    r |= spec_to_int8( spec + 4 ) << 8;
    r |= spec_to_int8( spec + 6 ) << 0;
    return r;
}

constexpr int spec_to_int16( const char* spec )
{
    int r = 0;
    r |= spec_to_int8( spec + 0 ) << 8;
    r |= spec_to_int8( spec + 2 ) << 0;
    return r;
}

#define _Maybenull_
#define CROSS_PLATFORM_UUIDOF( interface, spec ) \
    struct interface;                           \
    __CRT_UUID_DECL( interface,                 \
        (unsigned int)spec_to_int32(   (spec)+0),        \
        (unsigned short)spec_to_int16( (spec)+9),        \
        (unsigned short)spec_to_int16( (spec)+14),       \
        (unsigned char)spec_to_int8(   (spec)+19),       \
        (unsigned char)spec_to_int8(   (spec)+21),       \
        (unsigned char)spec_to_int8(   (spec)+24),       \
        (unsigned char)spec_to_int8(   (spec)+26),       \
        (unsigned char)spec_to_int8(   (spec)+28),       \
        (unsigned char)spec_to_int8(   (spec)+30),       \
        (unsigned char)spec_to_int8(   (spec)+32),       \
        (unsigned char)spec_to_int8(   (spec)+34)        \
        );

#define DXC_API_IMPORT __attribute__ ((visibility ("default")))

#include <dxc/dxcapi.h>
#include <d3d12shader.h>

namespace zp
{
    enum ShaderOutputReflectionResourceType : zp_uint8_t
    {
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_CBUFFER,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_TBUFFER,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_TEXTURE,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_SAMPLER,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWTYPED,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_STRUCTURED,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWSTRUCTURED,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_BYTEADDRESS,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWBYTEADDRESS,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_APPEND_STRUCTURED,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_COMSUMED_STRUCTURED,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWSTRUCTURED_WITH_COUNTER,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE,
        ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_FEEDBACK_TEXTURE,
    };

    enum ShaderOutputReflectionDimension : zp_uint8_t
    {
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_UNKNOWN,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_BUFFER,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE1D,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE1D_ARRAY,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2D,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2D_ARRAY,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2DMS,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2DMS_ARRAY,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE3D,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURECUBE,
        ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURECUBE_ARRAY,
    };

    enum ShaderOutputReflectionReturnType : zp_uint8_t
    {
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_UNKNOWN,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_UNORM,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_SNORM,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_SINT,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_UINT,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_FLOAT,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_MIXED,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_DOUBLE,
        ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_CONTINUED,
    };

    enum ShaderOutputReflectionCBufferType : zp_uint8_t
    {
        ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_CBUFFER,
        ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_TBUFFER,
        ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_INTERFACE_POINTERS,
        ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_RESOURCE_BIND_INFO,
    };

    enum ShaderOutputReflectionComponentType : zp_uint8_t
    {
        ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_UNKNOWN,
        ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_UINT32,
        ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_INT32,
        ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_FLOAT32,
    };

    enum ShaderOutputReflectionInputOutputName : zp_uint8_t
    {
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_POSITION,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_CLIP_DISTANCE,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_CULL_DISTANCE,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_RENDER_TARGET_ARRAY_INDEX,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_VIEWPORT_ARRAY_INDEX,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_VERTEX_ID,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_PRIMITIVE_ID,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_INSTANCE_ID,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_IS_FRONT_FACE,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_SAMPLE_INDEX,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_BARYCENTRICS,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_SHADING_RATE,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_CULL_PRIMITIVE,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_TARGET,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_DEPTH,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_COVERAGE,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_DEPTH_GREATER_EQUAL,
        ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_DEPTH_LESS_EQUAL,
    };

    enum ShaderOutputReflectionMask : zp_uint8_t
    {
        // @formatter:off
        ZP_SHADER_OUTPUT_REFLECTION_MASK_NONE = 0,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_X = 1 << 0,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_Y = 1 << 1,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_Z = 1 << 2,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_W = 1 << 3,

        ZP_SHADER_OUTPUT_REFLECTION_MASK_XY =   ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_Y,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_XYZ =  ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_Y | ZP_SHADER_OUTPUT_REFLECTION_MASK_Z,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_XYZW = ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_Y | ZP_SHADER_OUTPUT_REFLECTION_MASK_Z | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,

        ZP_SHADER_OUTPUT_REFLECTION_MASK_XZ =  ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_Z,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_XW =  ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_XYW = ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_Y | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_XZW = ZP_SHADER_OUTPUT_REFLECTION_MASK_X | ZP_SHADER_OUTPUT_REFLECTION_MASK_Z | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,

        ZP_SHADER_OUTPUT_REFLECTION_MASK_YZ =  ZP_SHADER_OUTPUT_REFLECTION_MASK_Y | ZP_SHADER_OUTPUT_REFLECTION_MASK_Z,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_YW =  ZP_SHADER_OUTPUT_REFLECTION_MASK_Y | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,
        ZP_SHADER_OUTPUT_REFLECTION_MASK_YZW = ZP_SHADER_OUTPUT_REFLECTION_MASK_Y | ZP_SHADER_OUTPUT_REFLECTION_MASK_Z | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,

        ZP_SHADER_OUTPUT_REFLECTION_MASK_ZW =  ZP_SHADER_OUTPUT_REFLECTION_MASK_Z | ZP_SHADER_OUTPUT_REFLECTION_MASK_W,

        ZP_SHADER_OUTPUT_REFLECTION_MASK_ALL = 0xFF,
        // @formatter:on
    };

    struct ShaderOutputReflectionHeader
    {
        zp_uint8_t resourceCount;
        zp_uint8_t elementCount;
        zp_uint8_t inputCount;
        zp_uint8_t outputCount;
    };

    struct ShaderOutputReflectionResource
    {
        FixedString<32> name;

        ShaderOutputReflectionResourceType type;
        ShaderOutputReflectionDimension dimension;
        ShaderOutputReflectionReturnType returnType;
        zp_uint8_t bindIndex;

        zp_uint8_t bindCount;
        zp_uint8_t numSamples;
        zp_uint8_t space;
        zp_uint8_t flags;

        zp_uint32_t size;
    };

    struct ShaderOutputReflectionElement
    {
        FixedString<32> name;
        zp_uint32_t startOffset;
        zp_uint32_t size;
        zp_uint32_t resourceIndex;

        zp_uint8_t textureIndex;
        zp_uint8_t textureCount;
        zp_uint8_t samplerIndex;
        zp_uint8_t samplerCount;
    };

    struct ShaderOutputReflectionInputOutput
    {
        FixedString<32> name;
        ShaderOutputReflectionInputOutputName type;
        zp_uint8_t semanticIndex;
        zp_uint8_t registerIndex;
        ShaderOutputReflectionComponentType componentType;

        zp_uint8_t stream;
        ShaderOutputReflectionMask mask;
        ShaderOutputReflectionMask readWriteMask;
        zp_uint8_t flags;
    };
}

#define HR( r )     do { hr = (r); ZP_ASSERT_MSG( SUCCEEDED(hr), #r ); } while( false )

namespace
{
    using namespace zp;

    template<typename T>
    struct Ptr
    {
        T* ptr;

        Ptr()
            : ptr( nullptr )
        {
        }

        explicit Ptr( T* t )
            : ptr( t )
        {
            if( ptr )
            {
                ptr->AddRef();
            }
        }

        Ptr( const Ptr& other )
            : ptr( other.ptr )
        {
            if( ptr )
            {
                ptr->AddRef();
            }
        }

        Ptr( Ptr&& other ) noexcept
            : ptr( other.ptr )
        {
            if( ptr )
            {
                ptr->Acquire();
            }

            if( other.ptr )
            {
                other.ptr->Release();
                other.ptr = nullptr;
            }
        }

        ~Ptr()
        {
            if( ptr )
            {
                ptr->Release();
                ptr = nullptr;
            }
        }

        Ptr& operator=( const Ptr& other )
        {
            if( ptr != other.ptr )
            {
                if( ptr )
                {
                    ptr->Release();
                    ptr = nullptr;
                }

                ptr = other.ptr;

                if( ptr )
                {
                    ptr->AddRef();
                }
            }

            return *this;
        }

        ZP_FORCEINLINE T* operator->()
        {
            return ptr;
        }

        ZP_FORCEINLINE const T* operator->() const
        {
            return ptr;
        }
    };

    enum ShaderProgramType
    {
        SHADER_PROGRAM_TYPE_VERTEX,
        SHADER_PROGRAM_TYPE_FRAGMENT,
        SHADER_PROGRAM_TYPE_GEOMETRY,
        SHADER_PROGRAM_TYPE_TESSELATION,
        SHADER_PROGRAM_TYPE_HULL,
        SHADER_PROGRAM_TYPE_DOMAIN,
        SHADER_PROGRAM_TYPE_TASK,
        SHADER_PROGRAM_TYPE_MESH,
        SHADER_PROGRAM_TYPE_COMPUTE,

        ShaderProgramType_Count,
    };

    enum ShaderProgramSupportedType
    {
        SHADER_PROGRAM_SUPPORTED_TYPE_NONE = 0u,
        SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX = 1 << SHADER_PROGRAM_TYPE_VERTEX,
        SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT = 1 << SHADER_PROGRAM_TYPE_FRAGMENT,
        SHADER_PROGRAM_SUPPORTED_TYPE_GEOMETRY = 1 << SHADER_PROGRAM_TYPE_GEOMETRY,
        SHADER_PROGRAM_SUPPORTED_TYPE_TESSELATION = 1 << SHADER_PROGRAM_TYPE_TESSELATION,
        SHADER_PROGRAM_SUPPORTED_TYPE_HULL = 1 << SHADER_PROGRAM_TYPE_HULL,
        SHADER_PROGRAM_SUPPORTED_TYPE_DOMAIN = 1 << SHADER_PROGRAM_TYPE_DOMAIN,
        SHADER_PROGRAM_SUPPORTED_TYPE_TASK = 1 << SHADER_PROGRAM_TYPE_TASK,
        SHADER_PROGRAM_SUPPORTED_TYPE_MESH = 1 << SHADER_PROGRAM_TYPE_MESH,
        SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE = 1 << SHADER_PROGRAM_TYPE_COMPUTE,
        SHADER_PROGRAM_SUPPORTED_TYPE_ALL = ~0u
    };

    const char* kShaderTypes[] {
        "vs",
        "ps",
        "gs",
        "ts",
        "hs",
        "ds",
        "as",
        "ms",
        "cs",
    };
    ZP_STATIC_ASSERT( ShaderProgramType_Count == ZP_ARRAY_SIZE( kShaderTypes ) );

    enum ShaderModelType
    {
        SHADER_MODEL_TYPE_4_0,
        SHADER_MODEL_TYPE_5_0,
        SHADER_MODEL_TYPE_5_1,
        SHADER_MODEL_TYPE_6_0,
        SHADER_MODEL_TYPE_6_1,
        SHADER_MODEL_TYPE_6_2,
        SHADER_MODEL_TYPE_6_3,
        SHADER_MODEL_TYPE_6_4,
        SHADER_MODEL_TYPE_6_5,
        SHADER_MODEL_TYPE_6_6,

        ShaderModelType_Count,
    };

    enum ShaderAPI
    {
        SHADER_API_D3D,
        SHADER_API_VULKAN,
    };

    enum ShaderOutput
    {
        SHADER_OUTPUT_DXIL,
        SHADER_OUTPUT_SPIRV,
    };

    struct ShaderModel
    {
        zp_uint8_t major;
        zp_uint8_t minor;
    };

    const ShaderModel kShaderModelTypes[] {
        { .major = 4, .minor = 0 },
        { .major = 5, .minor = 0 },
        { .major = 5, .minor = 1 },
        { .major = 6, .minor = 0 },
        { .major = 6, .minor = 1 },
        { .major = 6, .minor = 2 },
        { .major = 6, .minor = 3 },
        { .major = 6, .minor = 4 },
        { .major = 6, .minor = 5 },
        { .major = 6, .minor = 6 },
    };
    ZP_STATIC_ASSERT( ShaderModelType_Count == ZP_ARRAY_SIZE( kShaderModelTypes ) );

    enum
    {
        kEntryPointSize = 128,
        kMaxShaderOutputPathLength = 512,
        kMaxShaderFeaturesPerProgram = 8,
    };

    struct ShaderFeature
    {
        FixedArray<zp_size_t, kMaxShaderFeaturesPerProgram> shaderFeatures;
        zp_hash64_t shaderFeatureHash;
        zp_size_t shaderFeatureCount;
        zp_uint32_t shaderProgramSupportedType;
    };

    struct ShaderTaskData
    {
    public:
        explicit ShaderTaskData( MemoryLabel memoryLabel )
            : shaderSource()
            , entryPoints()
            , allShaderFeatures( 64, memoryLabel )
            , shaderFeatures( 64, memoryLabel )
            , invalidShaderFeatures( 8, memoryLabel )
            , memoryLabel( memoryLabel )
        {
        }

        AllocString shaderSource;

        FixedString64 entryPoints[ShaderProgramType_Count];

        Vector<String> allShaderFeatures;

        Vector<ShaderFeature> shaderFeatures;
        Vector<ShaderFeature> invalidShaderFeatures;

        zp_uint32_t shaderCompilerSupportedTypes;
        ShaderModelType shaderModel;

        ShaderAPI shaderAPI;
        ShaderOutput shaderOutput;

        ZP_BOOL32( debug );

    public:
        const MemoryLabel memoryLabel;
    };

    const char* kShaderEntryName[] {
        "Vertex",
        "Fragment",
        "Geometry",
        "Tesselation",
        "Hull",
        "Domain",
        "Task",
        "Mesh",
        "Compute",
    };
    ZP_STATIC_ASSERT( ShaderProgramType_Count == ZP_ARRAY_SIZE( kShaderEntryName ) );

    class LocalMalloc : public IMalloc
    {
    public:
        LocalMalloc()
            : memoryLabel( 0 )
        {
        };

        explicit LocalMalloc( zp::MemoryLabel memoryLabel )
            : memoryLabel( memoryLabel )
        {
        }

        void* Alloc( SIZE_T cb ) override
        {
            return ZP_MALLOC( memoryLabel, cb );
        }

        void* Realloc( void* pv, SIZE_T cb ) override
        {
            return ZP_REALLOC( memoryLabel, pv, cb );
        }

        void Free( void* pv ) override
        {
            ZP_FREE( memoryLabel, pv );
        }

        SIZE_T GetSize( void* pv ) override
        {
            return 0;
        }

        int DidAlloc( void* pv ) override
        {
            return 1;
        }

        void HeapMinimize() override
        {

        }

        HRESULT QueryInterface( const IID& riid, void** ppvObject ) override
        {
            return 0;
        }

        ULONG AddRef() override
        {
            return 0;
        }

        ULONG Release() override
        {
            return 0;
        }

    public:
        zp::MemoryLabel memoryLabel;
    };

    wchar_t* ConvertStrToWide( wchar_t*& buffer, const char* str )
    {
        int len = ::MultiByteToWideChar( CP_UTF8, 0, str, -1, nullptr, 0 );
        ::MultiByteToWideChar( CP_UTF8, 0, str, -1, buffer, len );
        buffer[ len ] = '\0';

        wchar_t* const wideStr = buffer;
        buffer += len + 1;

        return wideStr;
    }

    wchar_t* ConvertStrToWide( wchar_t*& buffer, const char* str, zp_size_t length )
    {
        int len = ::MultiByteToWideChar( CP_UTF8, 0, str, (int)length, nullptr, 0 );
        ::MultiByteToWideChar( CP_UTF8, 0, str, (int)length, buffer, len );
        buffer[ len ] = '\0';

        wchar_t* const wideStr = buffer;
        buffer += len + 1;

        return wideStr;
    }

    wchar_t* ConvertStrToWide( wchar_t*& buffer, const String& str )
    {
        return ConvertStrToWide( buffer, str.c_str(), str.length );
    }

    //
    //
    //

    constexpr ShaderOutputReflectionResourceType Convert( D3D_SHADER_INPUT_TYPE value )
    {
        constexpr ShaderOutputReflectionResourceType typeMap[] = {
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_CBUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_TBUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_TEXTURE,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_SAMPLER,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWTYPED,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_STRUCTURED,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWSTRUCTURED,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_BYTEADDRESS,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWBYTEADDRESS,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_APPEND_STRUCTURED,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_COMSUMED_STRUCTURED,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RWSTRUCTURED_WITH_COUNTER,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_RT_ACCELERATION_STRUCTURE,
            ZP_SHADER_OUTPUT_REFLECTION_RESOURCE_TYPE_FEEDBACK_TEXTURE,
        };
        return typeMap[ value ];
    }

    constexpr ShaderOutputReflectionDimension Convert( D3D_SRV_DIMENSION value )
    {
        constexpr ShaderOutputReflectionDimension typeMap[] = {
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_UNKNOWN,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_BUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE1D,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE1D_ARRAY,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2D,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2D_ARRAY,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2DMS,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE2DMS_ARRAY,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURE3D,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURECUBE,
            ZP_SHADER_OUTPUT_REFLECTION_DIMENSION_TEXTURECUBE_ARRAY,
        };
        return typeMap[ value ];
    }

    constexpr ShaderOutputReflectionReturnType Convert( D3D_RESOURCE_RETURN_TYPE value )
    {
        constexpr ShaderOutputReflectionReturnType typeMap[] = {
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_UNKNOWN,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_UNORM,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_SNORM,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_SINT,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_UINT,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_FLOAT,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_MIXED,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_DOUBLE,
            ZP_SHADER_OUTPUT_REFLECTION_RETURN_TYPE_CONTINUED,
        };
        return typeMap[ value ];
    }

    constexpr ShaderOutputReflectionCBufferType Convert( D3D_CBUFFER_TYPE value )
    {
        constexpr ShaderOutputReflectionCBufferType typeMap[] = {
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_CBUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_TBUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_INTERFACE_POINTERS,
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_RESOURCE_BIND_INFO,
        };
        return typeMap[ value ];
    }

    constexpr ShaderOutputReflectionComponentType Convert( D3D_REGISTER_COMPONENT_TYPE value )
    {
        ShaderOutputReflectionComponentType typeMap[] = {
            ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_UNKNOWN,
            ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_UINT32,
            ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_INT32,
            ZP_SHADER_OUTPUT_REFLECTION_COMPONENT_TYPE_FLOAT32,
        };
        return typeMap[ value ];
    }

    constexpr ShaderOutputReflectionInputOutputName Convert( D3D_NAME value )
    {
        // @formatter:off
        switch(value)
        {
            case D3D_NAME_UNDEFINED:                            return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_POSITION:                             return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_POSITION;
            case D3D_NAME_CLIP_DISTANCE:                        return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_CLIP_DISTANCE;
            case D3D_NAME_CULL_DISTANCE:                        return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_CULL_DISTANCE;
            case D3D_NAME_RENDER_TARGET_ARRAY_INDEX:            return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_RENDER_TARGET_ARRAY_INDEX;
            case D3D_NAME_VIEWPORT_ARRAY_INDEX:                 return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_VIEWPORT_ARRAY_INDEX;
            case D3D_NAME_VERTEX_ID:                            return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_VERTEX_ID;
            case D3D_NAME_PRIMITIVE_ID:                         return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_PRIMITIVE_ID;
            case D3D_NAME_INSTANCE_ID:                          return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_INSTANCE_ID;
            case D3D_NAME_IS_FRONT_FACE:                        return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_IS_FRONT_FACE;
            case D3D_NAME_SAMPLE_INDEX:                         return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_SAMPLE_INDEX;
            case D3D_NAME_FINAL_QUAD_EDGE_TESSFACTOR:           return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_FINAL_QUAD_INSIDE_TESSFACTOR:         return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_FINAL_TRI_EDGE_TESSFACTOR:            return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_FINAL_TRI_INSIDE_TESSFACTOR:          return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_FINAL_LINE_DETAIL_TESSFACTOR:         return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_FINAL_LINE_DENSITY_TESSFACTOR:        return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
            case D3D_NAME_BARYCENTRICS:                         return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_BARYCENTRICS;
            case D3D_NAME_SHADINGRATE:                          return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_SHADING_RATE;
            case D3D_NAME_CULLPRIMITIVE:                        return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_CULL_PRIMITIVE;
            case D3D_NAME_TARGET:                               return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_TARGET;
            case D3D_NAME_DEPTH:                                return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_DEPTH;
            case D3D_NAME_COVERAGE:                             return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_COVERAGE;
            case D3D_NAME_DEPTH_GREATER_EQUAL:                  return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_DEPTH_GREATER_EQUAL;
            case D3D_NAME_DEPTH_LESS_EQUAL:                     return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_DEPTH_LESS_EQUAL;
            default:                                            return ZP_SHADER_OUTPUT_REFLECTION_INPUT_OUTPUT_NAME_UNDEFINED;
        }
        // @formatter:on
    }

    constexpr ShaderOutputReflectionMask Convert( BYTE mask )
    {
        return ShaderOutputReflectionMask( mask );
    }

    //
    //
    //

    void PreprocessShader( const JobHandle& parentJob, ShaderTaskData* shaderTask )
    {

    }

    struct CompileJob
    {
        AssetCompilerTask* task;
        ShaderProgramType shaderProgram;
        ShaderFeature shaderFeature;
        FixedString64 entryPoint;

        zp_int32_t result;
        zp_hash128_t resultShaderHash;
        zp_hash64_t resultShaderFeatureHash;
        MutableFixedString512 resultShaderFilePath;
    };

    struct DxcCompilerThreadData
    {
        IDxcUtils* utils;
        IDxcIncludeHandler* includeHandler;
        IDxcCompiler3* compiler;
        LocalMalloc malloc;
    };

    thread_local DxcCompilerThreadData t_dxc {};

    void CompileShaderDXC( const JobHandle& parentJob, CompileJob* compilerJobData )
    {
        AssetCompilerTask* task = compilerJobData->task;

        HRESULT hr;
        const zp_uint32_t currentThreadId = zp_current_thread_id();

        zp_bool_t debugDisplay = true;

        if( t_dxc.utils == nullptr )
        {
            zp_printfln( "[%8d] Create DXC", currentThreadId );

            t_dxc.malloc.memoryLabel = task->memoryLabel;

            HR( DxcCreateInstance2( &t_dxc.malloc, CLSID_DxcUtils, IID_PPV_ARGS( &t_dxc.utils ) ) );

            HR( t_dxc.utils->CreateDefaultIncludeHandler( &t_dxc.includeHandler ) );

            HR( DxcCreateInstance2( &t_dxc.malloc, CLSID_DxcCompiler, IID_PPV_ARGS( &t_dxc.compiler ) ) );
        }

        Ptr<IDxcUtils> utils( t_dxc.utils );
        Ptr<IDxcIncludeHandler> includeHandler( t_dxc.includeHandler );
        Ptr<IDxcCompiler3> compiler( t_dxc.compiler );

        ShaderTaskData* data = task->taskMemory.as<ShaderTaskData>();

        Ptr<IDxcBlobEncoding> shaderSourceBlob;
        HR( utils->CreateBlob( data->shaderSource.c_str(), data->shaderSource.length(), DXC_CP_UTF8, &shaderSourceBlob.ptr ) );

        Ptr<IDxcBlobUtf8> shaderSourceUtf8Blob;
        HR( utils->GetBlobAsUtf8( shaderSourceBlob.ptr, &shaderSourceUtf8Blob.ptr ) );

        DxcBuffer sourceCode {
            .Ptr = shaderSourceUtf8Blob->GetBufferPointer(),
            .Size = shaderSourceUtf8Blob->GetBufferSize(),
            .Encoding = DXC_CP_UTF8,
        };

        //const AllocString srcFile = task->srcFile;
        //const AllocString dstFile = task->dstFile;

        const ShaderModel sm = kShaderModelTypes[ data->shaderModel ];

        wchar_t strBuffer[4 KB];
        wchar_t* strBufferPtr = strBuffer;

        char targetBuffer[8];
        zp_snprintf( targetBuffer, "%s_%d_%d", kShaderTypes[ compilerJobData->shaderProgram ], sm.major, sm.minor );

        wchar_t* targetProfile = ConvertStrToWide( strBufferPtr, targetBuffer );

        Vector<LPCWSTR> arguments( 24, task->memoryLabel );
        if( data->debug )
        {
            arguments.pushBack( DXC_ARG_DEBUG );
            arguments.pushBack( DXC_ARG_SKIP_OPTIMIZATIONS );
        }
        else
        {
            arguments.pushBack( DXC_ARG_OPTIMIZATION_LEVEL3 );
            arguments.pushBack( L"-Qstrip_debug" );
        }

        arguments.pushBack( DXC_ARG_WARNINGS_ARE_ERRORS );

        //arguments.pushBack( L"-remove-unused-functions" );
        //arguments.pushBack( L"-remove-unused-globals" );
        //arguments.pushBack( L"-keep-user-macro" );
        //arguments.pushBack( L"-line-directive" );

        arguments.pushBack( DXC_ARG_DEBUG_NAME_FOR_SOURCE );

        //arguments.pushBack( L"-P" );

        arguments.pushBack( L"-ftime-report" );
        arguments.pushBack( L"-ftime-trace" );

        arguments.pushBack( L"-Qstrip_debug" );
        //arguments.pushBack( L"-Qstrip_priv" );
        //arguments.pushBack( L"-Qstrip_reflect" );
        //arguments.pushBack( L"-Qstrip_rootsignature" );

        arguments.pushBack( L"-I" );
        arguments.pushBack( L"Assets/ShaderLibrary/" );

        switch( data->shaderOutput )
        {
            case SHADER_OUTPUT_DXIL:
                break;

            case SHADER_OUTPUT_SPIRV:
                arguments.pushBack( L"-spirv" );
                arguments.pushBack( L"-fspv-entrypoint-name=main" );
                arguments.pushBack( L"-fspv-reflect" );
                arguments.pushBack( L"-fvk-use-dx-position-w" );
                break;
        }

        const wchar_t* oneStr = L"1";

        Vector<DxcDefine> defines( 16, data->memoryLabel );

        // add shader API define
        switch( data->shaderAPI )
        {
            case SHADER_API_D3D:
                defines.pushBack( { .Name = L"SHADER_API_D3D", .Value = oneStr } );
                break;
            case SHADER_API_VULKAN:
                defines.pushBack( { .Name = L"SHADER_API_VULKAN", .Value = oneStr } );
                break;
        }

        // add feature defines
        for( zp_size_t i = 0; i < compilerJobData->shaderFeature.shaderFeatureCount; ++i )
        {
            const zp_size_t featureIndex = compilerJobData->shaderFeature.shaderFeatures[ i ];
            if( featureIndex > 0 )
            {
                const String& featureDefine = data->allShaderFeatures[ featureIndex ];

                defines.pushBack( {
                    .Name = ConvertStrToWide( strBufferPtr, featureDefine ),
                    .Value = oneStr
                } );
            }
        }

        // add debug define
        if( data->debug )
        {
            defines.pushBack( { .Name= L"DEBUG", .Value = oneStr } );
        }

        Ptr<IDxcResult> result;

        {
            auto srcFileName = ConvertStrToWide( strBufferPtr, task->srcFile.c_str() );
            auto entryPointName = ConvertStrToWide( strBufferPtr, compilerJobData->entryPoint.c_str() );

            Ptr<IDxcCompilerArgs> args;
            utils->BuildArguments( srcFileName, entryPointName, targetProfile, arguments.begin(), arguments.size(), defines.begin(), defines.size(), &args.ptr );

            if( debugDisplay )
            {
                MutableFixedString<2 KB> argsBuffer;
                auto f = args->GetArguments();

                argsBuffer.appendFormat( "[%8d] Args: ", currentThreadId );

                argsBuffer.append( '"' );
                argsBuffer.appendFormat( "%ls", f[ 0 ] );
                for( zp_size_t i = 1; i < args->GetCount(); ++i )
                {
                    argsBuffer.appendFormat( " %ls", f[ i ] );
                }
                argsBuffer.append( '"' );

                zp_printfln( argsBuffer.c_str() );
            }

            zp_printfln( "[%8d] Compile DXC...", currentThreadId );

            // compile
            HR( compiler->Compile( &sourceCode, args->GetArguments(), args->GetCount(), includeHandler.ptr, IID_PPV_ARGS( &result.ptr ) ) );
        }

        compilerJobData->result = 0;

        // check status
        HRESULT status;
        HR( result->GetStatus( &status ) );

        if( SUCCEEDED( status ) )
        {
            zp_printfln( "[%8d] ...Success", currentThreadId );

            compilerJobData->result = 1;

            // errors
            if( result->HasOutput( DXC_OUT_ERRORS ) )
            {
                Ptr<IDxcBlobUtf8> errorMsgs;
                HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                if( errorMsgs.ptr && errorMsgs->GetStringLength() )
                {
                    zp_printfln( "[ERROR] " " %s", errorMsgs->GetStringPointer() );
                }
            }

            // time report
            if( result->HasOutput( DXC_OUT_TIME_REPORT ) )
            {
                Ptr<IDxcBlobUtf8> timeReport;
                HR( result->GetOutput( DXC_OUT_TIME_REPORT, IID_PPV_ARGS( &timeReport.ptr ), nullptr ) );

                if( timeReport.ptr && timeReport->GetStringLength() )
                {
                    zp_printfln( "[REPORT]  " " %s", timeReport->GetStringPointer() );
                }
            }

            // time trace
            if( result->HasOutput( DXC_OUT_TIME_TRACE ) )
            {
                Ptr<IDxcBlobUtf8> timeTrace;
                HR( result->GetOutput( DXC_OUT_TIME_TRACE, IID_PPV_ARGS( &timeTrace.ptr ), nullptr ) );

                if( timeTrace.ptr && timeTrace->GetStringLength() )
                {
                    zp_printfln( "[TRACE]  " " %s", timeTrace->GetStringPointer() );
                }
            }

            // text
            if( result->HasOutput( DXC_OUT_TEXT ) )
            {
                Ptr<IDxcBlobUtf8> text;
                HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                if( text.ptr && text->GetStringLength() )
                {
                    zp_printfln( "[TEXT] " " %s", text->GetStringPointer() );
                }
            }

            // remarks
            if( result->HasOutput( DXC_OUT_REMARKS ) )
            {
                Ptr<IDxcBlobUtf8> remarks;
                HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                if( remarks.ptr && remarks->GetStringLength() )
                {
                    zp_printfln( "[REMARKS] " " %s", remarks->GetStringPointer() );
                }
            }

            // root signature
            if( result->HasOutput( DXC_OUT_ROOT_SIGNATURE ) )
            {
                Ptr<IDxcBlobUtf8> rootSignature;
                HR( result->GetOutput( DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS( &rootSignature.ptr ), nullptr ) );

                if( rootSignature.ptr && rootSignature->GetBufferSize() )
                {
                    zp_printfln( "[ROOTSIG] " " '%s'", rootSignature->GetStringPointer() );
                }
            }

            // hlsl
            if( result->HasOutput( DXC_OUT_HLSL ) )
            {
                Ptr<IDxcBlobUtf8> hlsl;
                HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                if( hlsl.ptr && hlsl->GetStringLength() )
                {
                    zp_printfln( "[HSLS] " " %s", hlsl->GetStringPointer() );
                }
            }

            // disassembly
            if( result->HasOutput( DXC_OUT_DISASSEMBLY ) )
            {
                Ptr<IDxcBlobUtf8> disassembly;
                HR( result->GetOutput( DXC_OUT_DISASSEMBLY, IID_PPV_ARGS( &disassembly.ptr ), nullptr ) );

                if( disassembly.ptr && disassembly->GetStringLength() )
                {
                    zp_printfln( "[DISASSEMBLY] " " %s", disassembly->GetStringPointer() );
                }
            }

            // disassembly
            if( result->HasOutput( DXC_OUT_EXTRA_OUTPUTS ) )
            {
                Ptr<IDxcExtraOutputs> extraOutput;
                HR( result->GetOutput( DXC_OUT_EXTRA_OUTPUTS, IID_PPV_ARGS( &extraOutput.ptr ), nullptr ) );

                if( extraOutput.ptr && extraOutput->GetOutputCount() )
                {
                    zp_printfln( "[EXTRA] " " %d", extraOutput->GetOutputCount() );
                }
            }

            //
            //
            //

            struct ShaderOutputHeader
            {
                zp_uint32_t id;
                zp_uint32_t shaderFeatureCount;
                zp_hash64_t shaderFeatureHash;
                zp_uint64_t pdbOffset;
                zp_uint64_t reflectionOffset;
                zp_uint64_t shaderDataOffset;
                zp_hash128_t hash;
            };

            struct ShaderOutputShaderFeature
            {
                FixedString<32> name;
            };

            struct ShaderOutputPDBHeader
            {
                FixedString<64> filePath;
                zp_uint64_t size;
            };

            ShaderOutputHeader shaderOutputHeader {
                .id = zp_make_cc4( "ZPSH" ),
                .shaderFeatureCount = zp_uint32_t( compilerJobData->shaderFeature.shaderFeatureCount ),
                .shaderFeatureHash = compilerJobData->shaderFeature.shaderFeatureHash,
                .pdbOffset = 0,
                .reflectionOffset = 0,
                .shaderDataOffset = 0,
                .hash = {}
            };

            compilerJobData->resultShaderHash = {};

            DataStreamWriter shaderStream( data->memoryLabel, 4 KB );
            shaderStream.write( shaderOutputHeader );

            // write features
            for( zp_size_t i = 0; i < compilerJobData->shaderFeature.shaderFeatureCount; ++i )
            {
                const zp_size_t featureIndex = compilerJobData->shaderFeature.shaderFeatures[ i ];
                if( featureIndex > 0 )
                {
                    const String& featureDefine = data->allShaderFeatures[ featureIndex ];

                    ShaderOutputShaderFeature shaderFeature {
                        .name = FixedString<32>( featureDefine.c_str(), featureDefine.length )
                    };

                    shaderStream.write( shaderFeature );
                }
            }

            // pdb file and path
            if( result->HasOutput( DXC_OUT_PDB ) )
            {
                Ptr<IDxcBlob> pdbData;
                Ptr<IDxcBlobUtf16> pdbPathFromCompiler;
                HR( result->GetOutput( DXC_OUT_PDB, IID_PPV_ARGS( &pdbData.ptr ), &pdbPathFromCompiler.ptr ) );

                if( pdbData.ptr && pdbData->GetBufferSize() )
                {
                    char pdbFilePath[64];
                    int pathLength = ::WideCharToMultiByte( CP_UTF8, 0, pdbPathFromCompiler->GetStringPointer(), -1, pdbFilePath, ZP_ARRAY_SIZE( pdbFilePath ), nullptr, nullptr );

                    ShaderOutputPDBHeader pdbHeader {
                        .filePath = FixedString<64>( pdbFilePath ),
                        .size = pdbData->GetBufferSize()
                    };

                    shaderOutputHeader.pdbOffset = shaderStream.position();
                    shaderStream.write( pdbHeader );
                    shaderStream.write( pdbData->GetBufferPointer(), pdbData->GetBufferSize() );
                    shaderStream.writeAlignment( 16 );


                    //zp_handle_t pdbFileHandle = Platform::OpenFileHandle( pdbFilePath, ZP_OPEN_FILE_MODE_WRITE );
                    //Platform::WriteFile( pdbFileHandle, pdbData->GetBufferPointer(), pdbData->GetBufferSize() );
                    //Platform::CloseFileHandle( pdbFileHandle );

                    zp_printfln( "[PDB]  " " %s", pdbFilePath );
                }
            }

            // reflection
            if( result->HasOutput( DXC_OUT_REFLECTION ) )
            {
                Ptr<IDxcBlob> reflection;
                HR( result->GetOutput( DXC_OUT_REFLECTION, IID_PPV_ARGS( &reflection.ptr ), nullptr ) );

                zp_printfln( "[REFLECTION]  " " " );

                if( reflection.ptr )
                {
                    MutableFixedString<2 KB> info;

                    // save to file also
                    DxcBuffer reflectionData {
                        .Ptr = reflection->GetBufferPointer(),
                        .Size = reflection->GetBufferSize(),
                        .Encoding = DXC_CP_ACP
                    };

                    Ptr<ID3D12ShaderReflection> shaderReflection;
                    HR( utils->CreateReflection( &reflectionData, IID_PPV_ARGS( &shaderReflection.ptr ) ) );

                    if( shaderReflection.ptr )
                    {
#define INFO_D( d, n ) "%s: %d ", #n, d.n
#define INFO_S( d, n ) "%s: %s ", #n, d.n
#define INFO_X( d, n ) "%s: %02x ", #n, d.n

                        D3D12_SHADER_DESC shaderDesc;
                        HR( shaderReflection->GetDesc( &shaderDesc ) );

                        Vector<zp_hash64_t> resourceNameHashes( shaderDesc.BoundResources, data->memoryLabel );
                        Vector<ShaderOutputReflectionResource> resources( shaderDesc.BoundResources, data->memoryLabel );
                        Vector<ShaderOutputReflectionElement> elements( shaderDesc.BoundResources, data->memoryLabel );
                        Vector<ShaderOutputReflectionInputOutput> inputs( shaderDesc.InputParameters, data->memoryLabel );
                        Vector<ShaderOutputReflectionInputOutput> outputs( shaderDesc.OutputParameters, data->memoryLabel );

                        zp_printfln( "Bound Resources: %d ", shaderDesc.BoundResources );
                        for( zp_uint32_t i = 0; i < shaderDesc.BoundResources; ++i )
                        {
                            D3D12_SHADER_INPUT_BIND_DESC inputBindDesc;
                            HR( shaderReflection->GetResourceBindingDesc( i, &inputBindDesc ) );

                            ShaderOutputReflectionResource resource {
                                .name = FixedString<32>( inputBindDesc.Name ),
                                .type = Convert( inputBindDesc.Type ),
                                .dimension = Convert( inputBindDesc.Dimension ),
                                .returnType = Convert( inputBindDesc.ReturnType ),
                                .bindIndex = zp_uint8_t( inputBindDesc.BindPoint ),
                                .bindCount = zp_uint8_t( inputBindDesc.BindCount ),
                                .numSamples = zp_uint8_t( inputBindDesc.NumSamples ),
                                .space = zp_uint8_t( inputBindDesc.Space ),
                                .flags = zp_uint8_t( 0 ),
                                .size = 0
                            };

                            resources.pushBack( resource );
                            resourceNameHashes.pushBack( zp_fnv64_1a( resource.name.str(), resource.name.length() ) );

                            if( debugDisplay )
                            {
                                info.clear();
                                info.append( "  " );
                                info.appendFormat( INFO_S( inputBindDesc, Name ) );
                                info.appendFormat( INFO_D( inputBindDesc, Type ) );
                                info.appendFormat( INFO_D( inputBindDesc, BindPoint ) );
                                info.appendFormat( INFO_D( inputBindDesc, BindCount ) );
                                info.appendFormat( INFO_X( inputBindDesc, uFlags ) );
                                info.appendFormat( INFO_D( inputBindDesc, ReturnType ) );
                                info.appendFormat( INFO_D( inputBindDesc, Dimension ) );
                                info.appendFormat( INFO_D( inputBindDesc, NumSamples ) );
                                info.appendFormat( INFO_D( inputBindDesc, Space ) );
                                info.appendFormat( INFO_D( inputBindDesc, uID ) );

                                zp_printfln( info.c_str() );
                            }
                        }

                        zp_printfln( "Constant Buffers: %d ", shaderDesc.ConstantBuffers );
                        for( zp_uint32_t i = 0; i < shaderDesc.ConstantBuffers; ++i )
                        {
                            auto cb = shaderReflection->GetConstantBufferByIndex( i );

                            D3D12_SHADER_BUFFER_DESC cbDesc;
                            cb->GetDesc( &cbDesc );

                            FixedString<32> cbName( cbDesc.Name );
                            const zp_hash64_t cbNameHash = zp_fnv64_1a( cbName.str(), cbName.length() );

                            const zp_size_t resourceIndex = resourceNameHashes.indexOf( cbNameHash );
                            ZP_ASSERT( resourceIndex != ZP_NPOS );

                            // adjust cbuffer resource size
                            resources[ resourceIndex ].size = cbDesc.Size;

                            if( debugDisplay )
                            {
                                info.clear();
                                info.append( "  " );
                                info.appendFormat( INFO_S( cbDesc, Name ) );
                                info.appendFormat( INFO_D( cbDesc, Type ) );
                                info.appendFormat( INFO_D( cbDesc, Variables ) );
                                info.appendFormat( INFO_D( cbDesc, Size ) );
                                info.appendFormat( INFO_X( cbDesc, uFlags ) );

                                zp_printfln( info.c_str() );
                            }

                            for( zp_size_t v = 0; v < cbDesc.Variables; ++v )
                            {
                                auto variable = cb->GetVariableByIndex( v );

                                D3D12_SHADER_VARIABLE_DESC vDesc;
                                variable->GetDesc( &vDesc );

                                ShaderOutputReflectionElement element {
                                    .name = FixedString<32>( vDesc.Name ),
                                    .startOffset = vDesc.StartOffset,
                                    .size = vDesc.Size,
                                    .resourceIndex = zp_uint32_t( resourceIndex ),
                                    .textureIndex = zp_uint8_t( vDesc.StartTexture ),
                                    .textureCount = zp_uint8_t( vDesc.TextureSize ),
                                    .samplerIndex = zp_uint8_t( vDesc.StartSampler ),
                                    .samplerCount = zp_uint8_t( vDesc.SamplerSize ),
                                };

                                elements.pushBack( element );

                                if( debugDisplay )
                                {
                                    info.clear();
                                    info.append( "    " );
                                    info.appendFormat( INFO_S( vDesc, Name ) );
                                    info.appendFormat( INFO_D( vDesc, StartOffset ) );
                                    info.appendFormat( INFO_D( vDesc, Size ) );
                                    info.appendFormat( INFO_X( vDesc, uFlags ) );
                                    info.appendFormat( INFO_D( vDesc, DefaultValue ) );
                                    info.appendFormat( INFO_D( vDesc, StartTexture ) );
                                    info.appendFormat( INFO_D( vDesc, TextureSize ) );
                                    info.appendFormat( INFO_D( vDesc, StartSampler ) );
                                    info.appendFormat( INFO_D( vDesc, SamplerSize ) );

                                    zp_printfln( info.c_str() );
                                }
                            }
                        }

                        zp_printfln( "Input Parameters: %d ", shaderDesc.InputParameters );
                        for( zp_uint32_t i = 0; i < shaderDesc.InputParameters; ++i )
                        {
                            D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
                            HR( shaderReflection->GetInputParameterDesc( i, &parameterDesc ) );

                            ShaderOutputReflectionInputOutput input {
                                .name = FixedString<32>( parameterDesc.SemanticName ),
                                .type = Convert( parameterDesc.SystemValueType ),
                                .semanticIndex = zp_uint8_t( parameterDesc.SemanticIndex ),
                                .registerIndex = zp_uint8_t( parameterDesc.Register ),
                                .componentType = Convert( parameterDesc.ComponentType ),
                                .stream = zp_uint8_t( parameterDesc.Stream ),
                                .mask = Convert( parameterDesc.Mask ),
                                .readWriteMask = Convert( parameterDesc.ReadWriteMask ),
                            };

                            inputs.pushBack( input );

                            if( debugDisplay )
                            {
                                info.clear();
                                info.append( "  " );
                                info.appendFormat( INFO_S( parameterDesc, SemanticName ) );
                                info.appendFormat( INFO_D( parameterDesc, SemanticIndex ) );
                                info.appendFormat( INFO_D( parameterDesc, Register ) );
                                info.appendFormat( INFO_D( parameterDesc, SystemValueType ) );
                                info.appendFormat( INFO_D( parameterDesc, ComponentType ) );
                                info.appendFormat( INFO_X( parameterDesc, Mask ) );
                                info.appendFormat( INFO_X( parameterDesc, ReadWriteMask ) );
                                info.appendFormat( INFO_D( parameterDesc, Stream ) );
                                info.appendFormat( INFO_D( parameterDesc, MinPrecision ) );

                                zp_printfln( info.c_str() );
                            }
                        }

                        zp_printfln( "Output Parameters: %d ", shaderDesc.OutputParameters );
                        for( zp_uint32_t i = 0; i < shaderDesc.OutputParameters; ++i )
                        {
                            D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
                            HR( shaderReflection->GetOutputParameterDesc( i, &parameterDesc ) );

                            ShaderOutputReflectionInputOutput output {
                                .name = FixedString<32>( parameterDesc.SemanticName ),
                                .type = Convert( parameterDesc.SystemValueType ),
                                .semanticIndex = zp_uint8_t( parameterDesc.SemanticIndex ),
                                .registerIndex = zp_uint8_t( parameterDesc.Register ),
                                .componentType = Convert( parameterDesc.ComponentType ),
                                .stream = zp_uint8_t( parameterDesc.Stream ),
                                .mask = Convert( parameterDesc.Mask ),
                                .readWriteMask = Convert( parameterDesc.ReadWriteMask ),
                            };

                            outputs.pushBack( output );

                            if( debugDisplay )
                            {
                                info.clear();
                                info.append( "  " );
                                info.appendFormat( INFO_S( parameterDesc, SemanticName ) );
                                info.appendFormat( INFO_D( parameterDesc, SemanticIndex ) );
                                info.appendFormat( INFO_D( parameterDesc, Register ) );
                                info.appendFormat( INFO_D( parameterDesc, SystemValueType ) );
                                info.appendFormat( INFO_D( parameterDesc, ComponentType ) );
                                info.appendFormat( INFO_X( parameterDesc, Mask ) );
                                info.appendFormat( INFO_X( parameterDesc, ReadWriteMask ) );
                                info.appendFormat( INFO_D( parameterDesc, Stream ) );
                                info.appendFormat( INFO_D( parameterDesc, MinPrecision ) );

                                zp_printfln( info.c_str() );
                            }
                        }

                        // write reflection block
                        shaderOutputHeader.reflectionOffset = shaderStream.position();

                        ShaderOutputReflectionHeader reflectionHeader {
                            .resourceCount = zp_uint8_t( resources.size() ),
                            .elementCount = zp_uint8_t( elements.size() ),
                            .inputCount = zp_uint8_t( inputs.size() ),
                            .outputCount = zp_uint8_t( outputs.size() ),
                        };

                        shaderStream.write( reflectionHeader );

                        resources.foreach( [ &shaderStream ]( const ShaderOutputReflectionResource& resource ) -> void
                        {
                            shaderStream.write( resource );
                        } );

                        elements.foreach( [ &shaderStream ]( const ShaderOutputReflectionElement& element ) -> void
                        {
                            shaderStream.write( element );
                        } );

                        inputs.foreach( [ &shaderStream ]( const ShaderOutputReflectionInputOutput& input ) -> void
                        {
                            shaderStream.write( input );
                        } );

                        outputs.foreach( [ &shaderStream ]( const ShaderOutputReflectionInputOutput& output ) -> void
                        {
                            shaderStream.write( output );
                        } );

                        shaderStream.writeAlignment( 16 );

                        // display stat info
                        if( debugDisplay )
                        {
#undef INFO_D
#undef INFO_S
#undef INFO_X

#define INFO_D( n ) "%30s: %d\n", #n, shaderDesc.n
#define INFO_S( n ) "%30s: %s\n", #n, shaderDesc.n

                            info.clear();
                            info.appendFormat( INFO_D( Version ) );
                            info.appendFormat( INFO_S( Creator ) );
                            info.appendFormat( INFO_D( ConstantBuffers ) );
                            info.appendFormat( INFO_D( BoundResources ) );
                            info.appendFormat( INFO_D( InputParameters ) );
                            info.appendFormat( INFO_D( OutputParameters ) );
                            info.appendFormat( INFO_D( InstructionCount ) );
                            info.appendFormat( INFO_D( TempRegisterCount ) );
                            info.appendFormat( INFO_D( TempArrayCount ) );
                            info.appendFormat( INFO_D( DefCount ) );
                            info.appendFormat( INFO_D( DclCount ) );
                            info.appendFormat( INFO_D( TextureNormalInstructions ) );
                            info.appendFormat( INFO_D( TextureLoadInstructions ) );
                            info.appendFormat( INFO_D( TextureCompInstructions ) );
                            info.appendFormat( INFO_D( TextureBiasInstructions ) );
                            info.appendFormat( INFO_D( TextureGradientInstructions ) );
                            info.appendFormat( INFO_D( FloatInstructionCount ) );
                            info.appendFormat( INFO_D( IntInstructionCount ) );
                            info.appendFormat( INFO_D( UintInstructionCount ) );
                            info.appendFormat( INFO_D( StaticFlowControlCount ) );
                            info.appendFormat( INFO_D( DynamicFlowControlCount ) );
                            info.appendFormat( INFO_D( MacroInstructionCount ) );
                            info.appendFormat( INFO_D( ArrayInstructionCount ) );
                            info.appendFormat( INFO_D( CutInstructionCount ) );
                            info.appendFormat( INFO_D( EmitInstructionCount ) );
                            info.appendFormat( INFO_D( GSOutputTopology ) );
                            info.appendFormat( INFO_D( GSMaxOutputVertexCount ) );
                            info.appendFormat( INFO_D( InputPrimitive ) );
                            info.appendFormat( INFO_D( PatchConstantParameters ) );
                            info.appendFormat( INFO_D( cGSInstanceCount ) );
                            info.appendFormat( INFO_D( cControlPoints ) );
                            info.appendFormat( INFO_D( HSOutputPrimitive ) );
                            info.appendFormat( INFO_D( HSPartitioning ) );
                            info.appendFormat( INFO_D( TessellatorDomain ) );
                            info.appendFormat( INFO_D( cBarrierInstructions ) );
                            info.appendFormat( INFO_D( cInterlockedInstructions ) );
                            info.appendFormat( INFO_D( cTextureStoreInstructions ) );
#undef INFO_D
#undef INFO_S

                            zp_printfln( info.c_str() );
                        }
                    }
                }
            }

            // shader hash
            if( result->HasOutput( DXC_OUT_SHADER_HASH ) )
            {
                Ptr<IDxcBlob> shaderHash;
                HR( result->GetOutput( DXC_OUT_SHADER_HASH, IID_PPV_ARGS( &shaderHash.ptr ), nullptr ) );

                if( shaderHash.ptr && shaderHash->GetBufferSize() )
                {
                    DxcShaderHash* dxcShaderHash = static_cast<DxcShaderHash*>(shaderHash->GetBufferPointer());

                    MutableFixedString64 hashStr;
                    for( unsigned char digit : dxcShaderHash->HashDigest )
                    {
                        hashStr.appendFormat( "%02x", digit );
                    }

                    zp_try_parse_hash128( hashStr.c_str(), hashStr.length(), &compilerJobData->resultShaderHash );

                    if( debugDisplay )
                    {
                        zp_printfln( "[HASH] " " KeyHash: %s", hashStr.c_str() );
                    }
                }
            }

            // shader byte code
            if( result->HasOutput( DXC_OUT_OBJECT ) )
            {
                Ptr<IDxcBlob> shaderObj;
                HR( result->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &shaderObj.ptr ), nullptr ) );

                struct ShaderOutputDataHeader
                {
                    zp_hash128_t hash;
                    zp_uint64_t size;
                    zp_uint64_t compressedSize;
                };

                const zp_size_t shaderObjSize = shaderObj->GetBufferSize();
                const void* shaderObjPtr = shaderObj->GetBufferPointer();

                ShaderOutputDataHeader dataHeader {
                    .hash = compilerJobData->resultShaderHash,
                    .size = shaderObjSize,
                    .compressedSize = 0,
                };

                zp_bool_t compressShaderBinary = false;

                shaderOutputHeader.shaderDataOffset = shaderStream.position();

                if( compressShaderBinary )
                {
                    AllocMemory compressedShaderBinary( data->memoryLabel, dataHeader.size );

                    const zp_size_t compressedSize = zp_lzf_compress( shaderObjPtr, 0, shaderObjSize, compressedShaderBinary.ptr, compressedShaderBinary.size );
                    dataHeader.compressedSize = compressedSize;

                    shaderStream.write( dataHeader );
                    shaderStream.write( compressedShaderBinary.ptr, compressedSize );
                }
                else
                {
                    shaderStream.write( dataHeader );
                    shaderStream.write( shaderObjPtr, shaderObjSize );
                }

                shaderStream.writeAlignment( 16 );

                //zp_handle_t dstFileHandle = Platform::OpenFileHandle( task->dstFilePath.c_str(), ZP_OPEN_FILE_MODE_WRITE );
                //Platform::WriteFile( dstFileHandle, shaderObj->GetBufferPointer(), shaderObj->GetBufferSize() );
                //Platform::CloseFileHandle( dstFileHandle );
                if( debugDisplay )
                {
                    zp_printfln( "[OBJECT] " " '%s'", task->dstFile.c_str() );
                }
            }

            const Memory shaderStreamMemory = shaderStream.memory();

            // hash file
            shaderOutputHeader.hash = zp_fnv128_1a( shaderStreamMemory.ptr, shaderStreamMemory.size );

            // rewrite header with values
            shaderStream.writeAt( shaderOutputHeader, 0, DataStreamSeekOrigin::Beginning );

            // write to file
            char hashStr[32 + 1];
            zp_try_hash128_to_string( compilerJobData->resultShaderHash, hashStr );

            char featureHashStr[16 + 1];
            zp_try_hash64_to_string( compilerJobData->shaderFeature.shaderFeatureHash, featureHashStr );

            char currentDir[512];
            String currentDirPath = Platform::GetCurrentDirStr( currentDir );

            zp_bool_t r;

            MutableFixedString512 dstFilePath;
            dstFilePath.append( currentDirPath );
            dstFilePath.append( Platform::PathSep );
            dstFilePath.append( "Library" );
            dstFilePath.append( Platform::PathSep );

            r = Platform::CreateDirectory( dstFilePath.c_str() );
            ZP_ASSERT_MSG_ARGS( r, "dir: %s", dstFilePath.c_str() );

            dstFilePath.append( "ShaderCache" );
            dstFilePath.append( Platform::PathSep );

            r = Platform::CreateDirectory( dstFilePath.c_str() );
            ZP_ASSERT_MSG_ARGS( r, "dir: %s", dstFilePath.c_str() );

            const char* srcFileName = zp_strrchr( task->srcFile.c_str(), Platform::PathSep ) + 1;

            dstFilePath.append( srcFileName );
            dstFilePath.append( Platform::PathSep );

            r = Platform::CreateDirectory( dstFilePath.c_str() );
            ZP_ASSERT_MSG_ARGS( r, "dir: %s", dstFilePath.c_str() );

            dstFilePath.append( hashStr );
            dstFilePath.append( '.' );
            dstFilePath.append( featureHashStr );
            dstFilePath.append( '.' );
            dstFilePath.append( kShaderTypes[ compilerJobData->shaderProgram ] );

            // store out resulting path
            compilerJobData->resultShaderFilePath = dstFilePath;
            compilerJobData->resultShaderFeatureHash = compilerJobData->shaderFeature.shaderFeatureHash;

            zp_handle_t dstFileHandle = Platform::OpenFileHandle( dstFilePath.c_str(), ZP_OPEN_FILE_MODE_WRITE );
            Platform::WriteFile( dstFileHandle, shaderStreamMemory.ptr, shaderStreamMemory.size );
            Platform::CloseFileHandle( dstFileHandle );
        }
        else
        {
            zp_printfln( "[%8d] ...Failed", currentThreadId );

            compilerJobData->result = -1;

            // errors
            if( result->HasOutput( DXC_OUT_ERRORS ) )
            {
                Ptr<IDxcBlobUtf8> errorMsgs;
                HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                const zp_size_t l = errorMsgs->GetStringLength();
                if( l )
                {
                    auto c = errorMsgs->GetStringPointer();
                    zp_printfln( "[ERROR] " " %s", c );
                }
            }

            // text
            if( result->HasOutput( DXC_OUT_TEXT ) )
            {
                Ptr<IDxcBlobUtf8> text;
                HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                if( text.ptr && text->GetStringLength() )
                {
                    zp_printfln( "[TEXT]  " " %s", text->GetStringPointer() );
                }
            }

            // hlsl
            if( result->HasOutput( DXC_OUT_HLSL ) )
            {
                Ptr<IDxcBlobUtf8> hlsl;
                HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                if( hlsl.ptr && hlsl->GetStringLength() )
                {
                    zp_printfln( "[HLSL]  " " %s", hlsl->GetStringPointer() );
                }
            }

            // remarks
            if( result->HasOutput( DXC_OUT_REMARKS ) )
            {
                Ptr<IDxcBlobUtf8> remarks;
                HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                if( remarks.ptr && remarks->GetStringLength() )
                {
                    zp_printfln( "[REMARKS]  " " %s", remarks->GetStringPointer() );
                }
            }
        }
    }
}

//
//
//

namespace zp::ShaderCompiler
{
    void ShaderCompilerExecuteJob( const JobHandle& parentJob, AssetCompilerTask* task )
    {
        ShaderTaskData* data = task->taskMemory.as<ShaderTaskData>();

        const zp_bool_t useJobSystem = false;

        zp_uint64_t processedShaderFeatureCount = 0;

        Vector<CompileJob> compileJobs( 64, task->memoryLabel );

        // TODO: invert loop so each shader feature can be grouped into tuple of shader programs,
        //  that way shader feature set "A" = ["vs 1", "ps 1"], "B" = ["vs 1", "ps 2"] etc
        //  then the tuple can be written out to the archive only keeping the unique programs
        for( zp_uint32_t shaderProgram = 0; shaderProgram < ShaderProgramType_Count; ++shaderProgram )
        {
            const zp_uint32_t shaderProgramType = 1 << shaderProgram;
            if( ( data->shaderCompilerSupportedTypes & shaderProgramType ) == shaderProgramType )
            {
                for( zp_size_t s = 0; s < data->shaderFeatures.size(); ++s )
                {
                    const ShaderFeature& shaderFeature = data->shaderFeatures[ s ];

                    // check shader feature is supported for the program type
                    zp_bool_t shaderFeatureSupported = ( shaderFeature.shaderProgramSupportedType & shaderProgramType ) == shaderProgramType;

                    // check for invalid shader features
                    if( shaderFeatureSupported )
                    {
                        for( auto invalidShaderFeature : data->invalidShaderFeatures )
                        {
                            if( invalidShaderFeature.shaderFeatureHash == shaderFeature.shaderFeatureHash )
                            {
                                shaderFeatureSupported = false;
                                break;
                            }
                        }
                    }

                    // compile each valid shader feature
                    if( shaderFeatureSupported )
                    {
                        for( zp_size_t i = 0; i < shaderFeature.shaderFeatureCount; ++i )
                        {
                            ShaderFeature activeShaderFeature {
                                .shaderFeatureCount = 1,
                                .shaderProgramSupportedType = shaderFeature.shaderProgramSupportedType,
                            };
                            activeShaderFeature.shaderFeatures[ 0 ] = shaderFeature.shaderFeatures[ i ];

                            activeShaderFeature.shaderFeatureHash = zp_fnv64_1a( activeShaderFeature.shaderFeatures );

                            CompileJob job {
                                .task = task,
                                .shaderProgram = (ShaderProgramType)shaderProgram,
                                .shaderFeature = activeShaderFeature,
                                .entryPoint = data->entryPoints[ shaderProgram ]
                            };

                            compileJobs.pushBack( job );
                        }
                    }
                }
            }
        }

        JobHandle parentCompileJob = JobSystem::Start().schedule();

        // compile each job
        for( CompileJob& job : compileJobs )
        {
            if( useJobSystem )
            {
                JobSystem::Start( CompileShaderDXC, job, parentCompileJob ).schedule();
            }
            else
            {
                CompileShaderDXC( parentCompileJob, &job );
            }
        }

        JobSystem::ScheduleBatchJobs();
        parentCompileJob.complete();

        char featureHashStr[16 + 1];

        // combine output shaders into single file
        ArchiveBuilder shaderArchiveBuilder( task->memoryLabel );
        // TODO: maybe load the existing archive? could be cool to merge the old and new, but could cause issues

        for( const CompileJob& job : compileJobs )
        {
            //zp_try_hash64_to_string( job.resultShaderFeatureHash, featureHashStr );
            //shaderArchiveBuilder.addBlock(  );

            // TODO: write out shader feature tuple ["vs 1", "ps 1"], ["vs 1", "ps 2"], etc.
            //  using the shader feature as the key
            // TODO: write of shader programs ["vs 1", "vs 2", ..., "ps 1", "ps 2",...]
            //  using the program name as the key
        }

        // compile archive
        DataStreamWriter shaderDataStream( task->memoryLabel );
        auto r = shaderArchiveBuilder.compile( 0, shaderDataStream );

        // write out compiled file on success
        if( r == ArchiveBuilderResult::Success )
        {
            zp_printfln( "Compile Success: %s", task->dstFile.c_str() );

            const Memory shaderDataMemory = shaderDataStream.memory();

            zp_handle_t dstFileHandle = Platform::OpenFileHandle( task->dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
            Platform::WriteFile( dstFileHandle, shaderDataMemory.ptr, shaderDataMemory.size );
            Platform::CloseFileHandle( dstFileHandle );
        }
        else
        {
            zp_printfln( "Failed to compile shader data %s", task->dstFile.c_str() );
        }
    }

#if 0
    void ShaderCompilerExecute( AssetCompilerTask* task )
    {
        // Old code for reference
        DxcBuffer sourceCode {
            .Ptr = data->shaderSource.ptr,
            .Size = data->shaderSource.size,
            .Encoding = 0,
        };

        HRESULT hr;

        AllocString srcFile = task->srcFile;
        AllocString dstFile = task->dstFile;

        LocalMalloc malloc( task->memoryLabel );

        for( zp_uint32_t shaderType = 0; shaderType < ShaderProgramType_Count; ++shaderType )
        {
            if( data->shaderCompilerSupportedTypes & ( 1 << shaderType ) )
            {
                ShaderModel sm = kShaderModelTypes[ data->shaderModel ];

                char entryPoint[kEntryPointSize];
                if( data->entryPoints[ shaderType ].empty() )
                {
                    auto start = zp_strrchr( srcFile.c_str(), '\\' ) + 1;
                    auto end = zp_strrchr( srcFile.c_str(), '.' );

                    zp_snprintf( entryPoint, "%.*s%s", end - start, start, kShaderEntryName[ shaderType ] );
                }
                else
                {
                    zp_strcpy( entryPoint, ZP_ARRAY_SIZE( entryPoint ), data->entryPoints[ shaderType ].c_str() );
                }

                WCHAR targetProfiler[] {
                    kShaderTypes[ shaderType ][ 0 ],
                    kShaderTypes[ shaderType ][ 1 ],
                    L'_',
                    static_cast<wchar_t>('0' + sm.major),
                    L'_',
                    static_cast<wchar_t>('0' + sm.minor),
                    '\0'
                };

                Vector<LPCWSTR> arguments( 24, task->memoryLabel );
                if( data->debug )
                {
                    arguments.pushBack( DXC_ARG_DEBUG );
                    arguments.pushBack( DXC_ARG_SKIP_OPTIMIZATIONS );
                }
                else
                {
                    arguments.pushBack( DXC_ARG_OPTIMIZATION_LEVEL3 );
                }

                arguments.pushBack( DXC_ARG_WARNINGS_ARE_ERRORS );

                //arguments.pushBack( L"-remove-unused-functions" );
                //arguments.pushBack( L"-remove-unused-globals" );

                arguments.pushBack( DXC_ARG_DEBUG_NAME_FOR_BINARY );

                //arguments.pushBack( L"-P" );

                arguments.pushBack( L"-ftime-report" );
                arguments.pushBack( L"-ftime-trace" );

                arguments.pushBack( L"-Qstrip_debug" );
                //arguments.pushBack( L"-Qstrip_priv" );
                //arguments.pushBack( L"-Qstrip_reflect" );
                //arguments.pushBack( L"-Qstrip_rootsignature" );

                arguments.pushBack( L"-I" );
                arguments.pushBack( L"Assets/ShaderLibrary/" );

                switch( data->shaderOutput )
                {
                    case SHADER_OUTPUT_DXIL:
                        break;

                    case SHADER_OUTPUT_SPIRV:
                        arguments.pushBack( L"-fspv-entrypoint-name=main" );
                        arguments.pushBack( L"-fspv-reflect" );
                        arguments.pushBack( L"-spirv" );
                        break;
                }

                Vector<DxcDefine> defines( 4, 0 );
                switch( data->shaderAPI )
                {
                    case SHADER_API_D3D:
                        defines.pushBack( { .Name= L"SHADER_API_D3D", .Value = L"1" } );
                        break;
                    case SHADER_API_VULKAN:
                        defines.pushBack( { .Name= L"SHADER_API_VULKAN", .Value = L"1" } );
                        break;
                }

                if( data->debug )
                {
                    defines.pushBack( { .Name= L"DEBUG", .Value = L"1" } );
                }

                Ptr <IDxcUtils> utils;
                HR( DxcCreateInstance2( &malloc, CLSID_DxcUtils, IID_PPV_ARGS( &utils.ptr ) ) );

                Ptr <IDxcIncludeHandler> includeHandler;
                HR( utils->CreateDefaultIncludeHandler( &includeHandler.ptr ) );

                Ptr <IDxcCompiler3> compiler;
                HR( DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler.ptr ) ) );

                WCHAR srcFileName[512];
                ::MultiByteToWideChar( CP_UTF8, 0, task->srcFile.c_str(), -1, srcFileName, ZP_ARRAY_SIZE( srcFileName ) );

                WCHAR entryPointName[kEntryPointSize];
                ::MultiByteToWideChar( CP_UTF8, 0, entryPoint, -1, entryPointName, ZP_ARRAY_SIZE( entryPointName ) );

                Ptr <IDxcCompilerArgs> args;
                utils->BuildArguments( srcFileName, entryPointName, targetProfiler, arguments.begin(), arguments.size(), defines.begin(), defines.size(), &args.ptr );

                MutableFixedString<1024> argsBuffer;
                auto f = args->GetArguments();
                for( zp_size_t i = 0; i < args->GetCount(); ++i )
                {
                    argsBuffer.appendFormat( "%ls ", f[ i ] );
                }
                argsBuffer.append( '\n' );

                zp_printfln( "Args: %s", argsBuffer.c_str() );

                // compile
                Ptr <IDxcResult> result;
                HR( compiler->Compile( &sourceCode, args->GetArguments(), args->GetCount(), includeHandler.ptr, IID_PPV_ARGS( &result.ptr ) ) );

                // check status
                HRESULT status;
                HR( result->GetStatus( &status ) );

                // compile successful
                if( SUCCEEDED( status ) )
                {
                    // errors
                    if( result->HasOutput( DXC_OUT_ERRORS ) )
                    {
                        Ptr <IDxcBlobUtf8> errorMsgs;
                        HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                        if( errorMsgs.ptr && errorMsgs->GetStringLength() )
                        {
                            zp_error_printfln( "[ERROR] " " %s", errorMsgs->GetStringPointer() );
                        }
                    }

                    // time report
                    if( result->HasOutput( DXC_OUT_TIME_REPORT ) )
                    {
                        Ptr <IDxcBlobUtf8> timeReport;
                        HR( result->GetOutput( DXC_OUT_TIME_REPORT, IID_PPV_ARGS( &timeReport.ptr ), nullptr ) );

                        if( timeReport.ptr && timeReport->GetStringLength() )
                        {
                            zp_printfln( "[REPORT]  " " %s", timeReport->GetStringPointer() );
                        }
                    }

                    // time trace
                    if( result->HasOutput( DXC_OUT_TIME_TRACE ) )
                    {
                        Ptr <IDxcBlobUtf8> timeTrace;
                        HR( result->GetOutput( DXC_OUT_TIME_TRACE, IID_PPV_ARGS( &timeTrace.ptr ), nullptr ) );

                        if( timeTrace.ptr && timeTrace->GetStringLength() )
                        {
                            zp_printfln( "[TRACE]  " " %s", timeTrace->GetStringPointer() );
                        }
                    }

                    // pdb file and path
                    if( result->HasOutput( DXC_OUT_PDB ) )
                    {
                        Ptr <IDxcBlob> pdbData;
                        Ptr <IDxcBlobUtf16> pdbPathFromCompiler;
                        HR( result->GetOutput( DXC_OUT_PDB, IID_PPV_ARGS( &pdbData.ptr ), &pdbPathFromCompiler.ptr ) );

                        if( pdbData.ptr && pdbData->GetBufferSize() )
                        {
                            char pdbFilePath[512];
                            ::WideCharToMultiByte( CP_UTF8, 0, pdbPathFromCompiler->GetStringPointer(), -1, pdbFilePath, ZP_ARRAY_SIZE( pdbFilePath ), nullptr, nullptr );

                            zp_handle_t pdbFileHandle = Platform::OpenFileHandle( pdbFilePath, ZP_OPEN_FILE_MODE_WRITE );
                            Platform::WriteFile( pdbFileHandle, pdbData->GetBufferPointer(), pdbData->GetBufferSize() );
                            Platform::CloseFileHandle( pdbFileHandle );

                            zp_printfln( "[PDB]  " " %s", pdbFilePath );
                        }
                    }

                    // reflection
                    if( result->HasOutput( DXC_OUT_REFLECTION ) )
                    {
                        Ptr <IDxcBlob> reflection;
                        HR( result->GetOutput( DXC_OUT_REFLECTION, IID_PPV_ARGS( &reflection.ptr ), nullptr ) );

                        zp_printfln( "[REFLECTION]  " " " );

                        if( reflection.ptr )
                        {
                            // save to file also
                            DxcBuffer reflectionData {
                                .Ptr = reflection->GetBufferPointer(),
                                .Size = reflection->GetBufferSize(),
                                .Encoding = 0
                            };

                            Ptr <ID3D12ShaderReflection> shaderReflection;
                            HR( utils->CreateReflection( &reflectionData, IID_PPV_ARGS( &shaderReflection.ptr ) ) );

                            if( shaderReflection.ptr )
                            {
                                D3D12_SHADER_DESC shaderDesc;
                                HR( shaderReflection->GetDesc( &shaderDesc ) );

                                zp_printfln( "Constant Buffers: %d ", shaderDesc.ConstantBuffers );
                                for( zp_uint32_t i = 0; i < shaderDesc.ConstantBuffers; ++i )
                                {
                                    auto cb = shaderReflection->GetConstantBufferByIndex( i );

                                    D3D12_SHADER_BUFFER_DESC cbDesc;
                                    cb->GetDesc( &cbDesc );

                                    zp_printfln( "  %s %d %d %d", cbDesc.Name, cbDesc.Type, cbDesc.Variables, cbDesc.Size );
                                }

                                zp_printfln( "Bound Resources: %d ", shaderDesc.BoundResources );
                                for( zp_uint32_t i = 0; i < shaderDesc.BoundResources; ++i )
                                {
                                    D3D12_SHADER_INPUT_BIND_DESC inputBindDesc;
                                    HR( shaderReflection->GetResourceBindingDesc( i, &inputBindDesc ) );

                                    zp_printfln( "  %s ", inputBindDesc.Name );
                                }

                                zp_printfln( "Input Parameters: %d ", shaderDesc.InputParameters );
                                for( zp_uint32_t i = 0; i < shaderDesc.InputParameters; ++i )
                                {
                                    D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
                                    HR( shaderReflection->GetInputParameterDesc( i, &parameterDesc ) );

                                    zp_printfln( "  %s ", parameterDesc.SemanticName );
                                }

                                zp_printfln( "Output Parameters: %d ", shaderDesc.OutputParameters );
                                for( zp_uint32_t i = 0; i < shaderDesc.OutputParameters; ++i )
                                {
                                    D3D12_SIGNATURE_PARAMETER_DESC parameterDesc;
                                    HR( shaderReflection->GetOutputParameterDesc( i, &parameterDesc ) );

                                    zp_printfln( "  %d %s", parameterDesc.SystemValueType, parameterDesc.SemanticName );
                                }

#define INFO_D( n ) "%30s: %d", #n, shaderDesc.n
#define INFO_S( n ) "%30s: %s", #n, shaderDesc.n

                                zp_printfln( INFO_D( Version ) );
                                zp_printfln( INFO_S( Creator ) );
                                zp_printfln( INFO_D( ConstantBuffers ) );
                                zp_printfln( INFO_D( BoundResources ) );
                                zp_printfln( INFO_D( InputParameters ) );
                                zp_printfln( INFO_D( OutputParameters ) );
                                zp_printfln( INFO_D( InstructionCount ) );
                                zp_printfln( INFO_D( TempRegisterCount ) );
                                zp_printfln( INFO_D( TempArrayCount ) );
                                zp_printfln( INFO_D( DefCount ) );
                                zp_printfln( INFO_D( DclCount ) );
                                zp_printfln( INFO_D( TextureNormalInstructions ) );
                                zp_printfln( INFO_D( TextureLoadInstructions ) );
                                zp_printfln( INFO_D( TextureCompInstructions ) );
                                zp_printfln( INFO_D( TextureBiasInstructions ) );
                                zp_printfln( INFO_D( TextureGradientInstructions ) );
                                zp_printfln( INFO_D( FloatInstructionCount ) );
                                zp_printfln( INFO_D( IntInstructionCount ) );
                                zp_printfln( INFO_D( UintInstructionCount ) );
                                zp_printfln( INFO_D( StaticFlowControlCount ) );
                                zp_printfln( INFO_D( DynamicFlowControlCount ) );
                                zp_printfln( INFO_D( MacroInstructionCount ) );
                                zp_printfln( INFO_D( ArrayInstructionCount ) );
                                zp_printfln( INFO_D( CutInstructionCount ) );
                                zp_printfln( INFO_D( EmitInstructionCount ) );
                                zp_printfln( INFO_D( GSOutputTopology ) );
                                zp_printfln( INFO_D( GSMaxOutputVertexCount ) );
                                zp_printfln( INFO_D( InputPrimitive ) );
                                zp_printfln( INFO_D( PatchConstantParameters ) );
                                zp_printfln( INFO_D( cGSInstanceCount ) );
                                zp_printfln( INFO_D( cControlPoints ) );
                                zp_printfln( INFO_D( HSOutputPrimitive ) );
                                zp_printfln( INFO_D( HSPartitioning ) );
                                zp_printfln( INFO_D( TessellatorDomain ) );
                                zp_printfln( INFO_D( cBarrierInstructions ) );
                                zp_printfln( INFO_D( cInterlockedInstructions ) );
                                zp_printfln( INFO_D( cTextureStoreInstructions ) );
#undef INFO_D
#undef INFO_S
                            }
                        }
                    }

                    // shader hash
                    if( result->HasOutput( DXC_OUT_SHADER_HASH ) )
                    {
                        Ptr <IDxcBlob> shaderHash;
                        HR( result->GetOutput( DXC_OUT_SHADER_HASH, IID_PPV_ARGS( &shaderHash.ptr ), nullptr ) );

                        if( shaderHash.ptr && shaderHash->GetBufferSize() )
                        {
                            DxcShaderHash* dxcShaderHash = static_cast<DxcShaderHash*>(shaderHash->GetBufferPointer());

                            char buff[32 + 1];
                            for( int i = 0; i < 16; ++i )
                            {
                                zp_snprintf( buff + ( i * 2 ), 3, "%02x", dxcShaderHash->HashDigest[ i ] );
                            }
                            buff[ 32 ] = 0;
                            zp_printfln( "[HASH] " " Hash: %s", buff );
                        }
                    }

                    // text
                    if( result->HasOutput( DXC_OUT_TEXT ) )
                    {
                        Ptr <IDxcBlobUtf8> text;
                        HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                        if( text.ptr && text->GetStringLength() )
                        {
                            zp_printfln( "[TEXT] " " %s", text->GetStringPointer() );
                        }
                    }

                    // remarks
                    if( result->HasOutput( DXC_OUT_REMARKS ) )
                    {
                        Ptr <IDxcBlobUtf8> remarks;
                        HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                        if( remarks.ptr && remarks->GetStringLength() )
                        {
                            zp_printfln( "[REMARKS] " " %s", remarks->GetStringPointer() );
                        }
                    }

                    // hlsl
                    if( result->HasOutput( DXC_OUT_HLSL ) )
                    {
                        Ptr <IDxcBlobUtf8> hlsl;
                        HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                        if( hlsl.ptr && hlsl->GetStringLength() )
                        {
                            zp_printfln( "[HSLS] " " %s", hlsl->GetStringPointer() );
                        }
                    }

                    // disassembly
                    if( result->HasOutput( DXC_OUT_DISASSEMBLY ) )
                    {
                        Ptr <IDxcBlobUtf8> disassembly;
                        HR( result->GetOutput( DXC_OUT_DISASSEMBLY, IID_PPV_ARGS( &disassembly.ptr ), nullptr ) );

                        if( disassembly.ptr && disassembly->GetStringLength() )
                        {
                            zp_printfln( "[DISASSEMBLY] " " %s", disassembly->GetStringPointer() );
                        }
                    }

                    // disassembly
                    if( result->HasOutput( DXC_OUT_EXTRA_OUTPUTS ) )
                    {
                        Ptr <IDxcExtraOutputs> extraOutput;
                        HR( result->GetOutput( DXC_OUT_EXTRA_OUTPUTS, IID_PPV_ARGS( &extraOutput.ptr ), nullptr ) );

                        if( extraOutput.ptr && extraOutput->GetOutputCount() )
                        {
                            zp_printfln( "[EXTRA] " " %d", extraOutput->GetOutputCount() );
                        }
                    }

                    // shader byte code
                    if( result->HasOutput( DXC_OUT_OBJECT ) )
                    {
                        Ptr <IDxcBlob> shaderObj;
                        HR( result->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &shaderObj.ptr ), nullptr ) );

                        zp_handle_t dstFileHandle = Platform::OpenFileHandle( dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
                        Platform::WriteFile( dstFileHandle, shaderObj->GetBufferPointer(), shaderObj->GetBufferSize() );
                        Platform::CloseFileHandle( dstFileHandle );

                        zp_printfln( "[OBJECT] " " %s", dstFile.c_str() );
                    }
                }
                else
                {
                    // errors
                    if( result->HasOutput( DXC_OUT_ERRORS ) )
                    {
                        Ptr <IDxcBlobUtf8> errorMsgs;
                        HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                        if( errorMsgs->GetStringLength() )
                        {
                            zp_error_printfln( "[ERROR] " " %s", errorMsgs->GetStringPointer() );
                        }
                    }

                    // text
                    if( result->HasOutput( DXC_OUT_TEXT ) )
                    {
                        Ptr <IDxcBlobUtf8> text;
                        HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                        if( text.ptr && text->GetStringLength() )
                        {
                            zp_printfln( "[TEXT]  " " %s", text->GetStringPointer() );
                        }
                    }

                    // hlsl
                    if( result->HasOutput( DXC_OUT_HLSL ) )
                    {
                        Ptr <IDxcBlobUtf8> hlsl;
                        HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                        if( hlsl.ptr && hlsl->GetStringLength() )
                        {
                            zp_printfln( "[HLSL]  " " %s", hlsl->GetStringPointer() );
                        }
                    }

                    // remarks
                    if( result->HasOutput( DXC_OUT_REMARKS ) )
                    {
                        Ptr <IDxcBlobUtf8> remarks;
                        HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                        if( remarks.ptr && remarks->GetStringLength() )
                        {
                            zp_printfln( "[REMARKS]  " " %s", remarks->GetStringPointer() );
                        }
                    }
                }
            }
        }

        //
        //
        //
        DataBuilder dataBuilder( 0 );

        dataBuilder.beginRecodring();

        dataBuilder.pushObject( String::As( "shader" ) );

        dataBuilder.popObject();

        dataBuilder.endRecording();
        //
        //
        //

        Memory compiledMemory = {};
        auto r = dataBuilder.compile( ZP_DATA_BUILDER_COMPILER_OPTION_NONE, compiledMemory );
        if( r == DataBuilderCompilerResult::Success )
        {

        }
    }
#endif

    Memory ShaderCompilerCreateTaskMemory( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine )
    {
        ShaderTaskData* data = nullptr;

        if( Platform::FileExists( inFile.c_str() ) )
        {
            data = ZP_NEW( memoryLabel, ShaderTaskData );

            // default values
            data->debug = false;
            data->shaderModel = SHADER_MODEL_TYPE_6_0;
            data->shaderCompilerSupportedTypes = 0;

            data->shaderAPI = SHADER_API_D3D;
            data->shaderOutput = SHADER_OUTPUT_DXIL;

            data->shaderSource = {};

            // add "empty" feature
            data->allShaderFeatures.pushBack( String::As( "_" ) );

            String ext = String::As( zp_strrchr( inFile.c_str(), '.' ) );
            zp_uint32_t requiredShaderPrograms = 0;

            if( zp_strcmp( ext, ".shader" ) == 0 )
            {
                requiredShaderPrograms = SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX | SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
            }
            else if( zp_strcmp( ext, ".compute" ) == 0 )
            {
                requiredShaderPrograms = SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE;
            }
            else
            {
                ZP_INVALID_CODE_PATH_MSG( "Unsupported shader file type used" );
            }

            zp_handle_t inFileHandle = Platform::OpenFileHandle( inFile.c_str(), ZP_OPEN_FILE_MODE_READ );
            const zp_size_t inFileSize = Platform::GetFileSize( inFileHandle );
            void* inFileData = ZP_MALLOC( memoryLabel, inFileSize );

            zp_size_t inFileReadSize = Platform::ReadFile( inFileHandle, inFileData, inFileSize );
            ZP_ASSERT( inFileSize == inFileReadSize );
            Platform::CloseFileHandle( inFileHandle );

            data->shaderSource.set( memoryLabel, (zp_char8_t*)inFileData, inFileSize ); //= { .str = (zp_char8_t*)inFileData, .length = inFileSize };

            // TODO: move to helper functions
            Tokenizer lineTokenizer( data->shaderSource.asString(), "\r\n" );

            String line {};
            while( lineTokenizer.next( line ) )
            {
                if( zp_strnstr( line, "#pragma " ) == line.c_str() )
                {
                    Tokenizer pragmaTokenizer( line, " " );

                    String pragma {};
                    while( pragmaTokenizer.next( pragma ) )
                    {
                        if( zp_strcmp( pragma, "vertex" ) == 0 )
                        {
                            data->shaderCompilerSupportedTypes |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                            data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ] = String::As( "" );

                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ] = pragmaOp;
                            }

                            zp_printfln( "Vertex Entry - %.*s", data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ].length(), data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ].c_str() );
                        }
                        else if( zp_strcmp( pragma, "fragment" ) == 0 )
                        {
                            data->shaderCompilerSupportedTypes |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ] = String::As( "" );

                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ] = pragmaOp;
                            }

                            zp_printfln( "Fragment Entry - %.*s", data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ].length(), data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ].c_str() );
                        }
                        else if( zp_strcmp( pragma, "kernel" ) == 0 )
                        {
                            data->shaderCompilerSupportedTypes |= SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE;
                            data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ] = String::As( "" );

                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ] = pragmaOp;
                            }

                            zp_printfln( "Compute Entry - %.*s", data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ].length(), data->entryPoints[ SHADER_PROGRAM_TYPE_COMPUTE ].c_str() );
                        }
                        else if( zp_strcmp( pragma, "enable_debug" ) == 0 )
                        {
                            data->debug = true;
                            zp_printfln( "Use Debug - true" );
                        }
                        else if( zp_strcmp( pragma, "target" ) == 0 )
                        {
                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                // format "#.#"
                                if( pragmaOp.length == 3 && pragmaOp.str[ 1 ] == '.' )
                                {
                                    ShaderModel model {};

                                    zp_bool_t ok = false;
                                    ok |= zp_try_parse_uint8( pragmaOp.c_str() + 0, 1, &model.major );
                                    ok |= zp_try_parse_uint8( pragmaOp.c_str() + 2, 1, &model.minor );
                                    if( ok )
                                    {
                                        zp_size_t index;
                                        if( zp_try_find_index( kShaderModelTypes, model, []( const ShaderModel& x, const ShaderModel& y )
                                        {
                                            return x.major == y.major && x.minor == y.minor;
                                        }, index ) )
                                        {
                                            data->shaderModel = static_cast<ShaderModelType>( index );

                                            zp_printfln( "Target - Major: %d Minor %d  Model Index %d", model.major, model.minor, data->shaderModel );
                                        }
                                    }
                                }
                            }
                        }
                            // TODO: have dxc precompile hlsl and then parse pragmas from that to support pragmas in included files, then compile variants from the preprocessed
                        else if( zp_strnstr( pragma, "shader_feature" ) != nullptr )
                        {
                            zp_uint32_t programSupportedType = SHADER_PROGRAM_SUPPORTED_TYPE_NONE;

                            if( zp_strcmp( pragma, "shader_feature_vertex" ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                            }
                            else if( zp_strcmp( pragma, "shader_feature_fragment" ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            }
                            else if( zp_strcmp( pragma, "shader_feature" ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            }

                            if( programSupportedType != 0 )
                            {
                                MutableFixedString<ShaderProgramType_Count> pp;
                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX )
                                {
                                    pp.append( "V" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT )
                                {
                                    pp.append( "F" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_GEOMETRY )
                                {
                                    pp.append( "G" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_TESSELATION )
                                {
                                    pp.append( "T" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_HULL )
                                {
                                    pp.append( "H" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_DOMAIN )
                                {
                                    pp.append( "D" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE )
                                {
                                    pp.append( "C" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_MESH )
                                {
                                    pp.append( "M" );
                                }

                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_TASK )
                                {
                                    pp.append( "A" );
                                }

                                zp_printfln( "Shader Programs %s:", pp.c_str() );

                                ShaderFeature shaderFeature { .shaderProgramSupportedType = programSupportedType };

                                // parse each feature
                                String pragmaOp {};
                                while( pragmaTokenizer.next( pragmaOp ) )
                                {
                                    ZP_ASSERT( shaderFeature.shaderFeatureCount < shaderFeature.shaderFeatures.length() );
                                    zp_printfln( "  - %.*s", pragmaOp.length, pragmaOp.str );

                                    // build feature list, add new one if it doesn't exist
                                    zp_size_t index;
                                    if( !zp_try_find_index( data->allShaderFeatures.begin(), data->allShaderFeatures.end(), pragmaOp, []( const String& x, const String& y )
                                    {
                                        return zp_strcmp( x.str, x.length, y.str, y.length ) == 0;
                                    }, index ) )
                                    {
                                        index = data->allShaderFeatures.size();
                                        data->allShaderFeatures.pushBack( pragmaOp );
                                    }

                                    shaderFeature.shaderFeatures[ shaderFeature.shaderFeatureCount++ ] = index;
                                }

                                // sort feature set in descending order
                                zp_qsort3( shaderFeature.shaderFeatures.begin(), shaderFeature.shaderFeatures.end(), zp_cmp_dsc<zp_size_t> );

                                // generate hash from sorted list
                                shaderFeature.shaderFeatureHash = zp_fnv64_1a( shaderFeature.shaderFeatures );

                                data->shaderFeatures.pushBack( shaderFeature );
                            }
                            break;
                        }
                        else if( zp_strnstr( pragma, "invalid_shader_feature" ) != nullptr )
                        {
                            zp_printfln( "Invalid Shader Features:" );

                            ShaderFeature shaderFeature { .shaderProgramSupportedType = SHADER_PROGRAM_SUPPORTED_TYPE_ALL };

                            // parse each feature
                            String pragmaOp {};
                            while( pragmaTokenizer.next( pragmaOp ) )
                            {
                                ZP_ASSERT( shaderFeature.shaderFeatureCount < shaderFeature.shaderFeatures.length() );
                                zp_printfln( "  - %.*s", pragmaOp.length, pragmaOp.str );

                                // build feature list, add new one if it doesn't exist
                                zp_size_t index;
                                if( !zp_try_find_index( data->allShaderFeatures.begin(), data->allShaderFeatures.end(), pragmaOp, []( const String& x, const String& y )
                                {
                                    return zp_strcmp( x.str, x.length, y.str, y.length ) == 0;
                                }, index ) )
                                {
                                    index = data->allShaderFeatures.size();
                                    data->allShaderFeatures.pushBack( pragmaOp );
                                }

                                shaderFeature.shaderFeatures[ shaderFeature.shaderFeatureCount++ ] = index;
                            }

                            // sort feature set
                            zp_qsort3( shaderFeature.shaderFeatures.begin(), shaderFeature.shaderFeatures.end(), zp_cmp_dsc<zp_size_t> );

                            // generate hash from sorted list
                            shaderFeature.shaderFeatureHash = zp_fnv64_1a( shaderFeature.shaderFeatures );

                            data->invalidShaderFeatures.pushBack( shaderFeature );
                        }
                    }
                }
            }

            ZP_ASSERT( ( data->shaderCompilerSupportedTypes & requiredShaderPrograms ) == requiredShaderPrograms );
        }

        return { .ptr = data, .size = data ? sizeof( ShaderTaskData ) : 0 };
    }

    void ShaderCompilerDestroyTaskMemory( MemoryLabel memoryLabel, Memory memory )
    {
        ZP_SAFE_DELETE( ShaderTaskData, memory.ptr );
    }
}
