//
// Created by phosg on 7/20/2023.
//

#include "Core/Allocator.h"
#include "Core/Data.h"
#include "Core/Job.h"
#include "Core/Vector.h"
#include "Core/String.h"
#include "Core/Log.h"

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

    const char* kShaderTypes[ ] {
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

    const ShaderModel kShaderModelTypes[ ] {
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

        FixedString64 entryPoints[ ShaderProgramType_Count ];

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

    const char* kShaderEntryName[ ] {
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
        return ConvertStrToWide( buffer, str.c_str(), str.length() );
    }

    //
    //
    //

    constexpr ShaderOutputReflectionResourceType Convert( D3D_SHADER_INPUT_TYPE value )
    {
        constexpr ShaderOutputReflectionResourceType typeMap[ ] = {
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
        constexpr ShaderOutputReflectionDimension typeMap[ ] = {
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
        constexpr ShaderOutputReflectionReturnType typeMap[ ] = {
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
        constexpr ShaderOutputReflectionCBufferType typeMap[ ] = {
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_CBUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_TBUFFER,
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_INTERFACE_POINTERS,
            ZP_SHADER_OUTPUT_REFLECTION_CBUFFER_TYPE_RESOURCE_BIND_INFO,
        };
        return typeMap[ value ];
    }

    constexpr ShaderOutputReflectionComponentType Convert( D3D_REGISTER_COMPONENT_TYPE value )
    {
        ShaderOutputReflectionComponentType typeMap[ ] = {
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
        //FilePath resultShaderFilePath;
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

        zp_bool_t debugDisplay = true;

        if( t_dxc.utils == nullptr )
        {
            Log::info() << "Created DXC" << Log::endl;

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

        wchar_t strBuffer[ 4 KB ];
        wchar_t* strBufferPtr = strBuffer;

        MutableFixedString16 target;
        target.appendFormat( "%s_%d_%d", kShaderTypes[ compilerJobData->shaderProgram ], sm.major, sm.minor );

        wchar_t* targetProfile = ConvertStrToWide( strBufferPtr, target.c_str() );

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

        const wchar_t* kOneStr = L"1";

        Vector<DxcDefine> defines( 16, data->memoryLabel );

        // add shader API define
        switch( data->shaderAPI )
        {
            case SHADER_API_D3D:
                defines.pushBack( { .Name = L"SHADER_API_D3D", .Value = kOneStr } );
                break;
            case SHADER_API_VULKAN:
                defines.pushBack( { .Name = L"SHADER_API_VULKAN", .Value = kOneStr } );
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
                    .Value = kOneStr
                } );
            }
        }

        // add debug define
        if( data->debug )
        {
            defines.pushBack( { .Name = L"DEBUG", .Value = kOneStr } );
        }

        Ptr<IDxcResult> result;

        {
            auto srcFileName = ConvertStrToWide( strBufferPtr, task->srcFile.c_str() );
            auto entryPointName = ConvertStrToWide( strBufferPtr, compilerJobData->entryPoint.c_str() );

            Ptr<IDxcCompilerArgs> args;
            utils->BuildArguments( srcFileName, entryPointName, targetProfile, arguments.begin(), arguments.length(), defines.begin(), defines.length(), &args.ptr );

            if( debugDisplay )
            {
                MutableFixedString<2 KB> argsBuffer;
                auto f = args->GetArguments();

                argsBuffer.appendFormat( "%ls", f[ 0 ] );
                for( zp_size_t i = 1, imax = args->GetCount(); i < imax; ++i )
                {
                    argsBuffer.appendFormat( " %ls", f[ i ] );
                }

                Log::info() << "Args: " << argsBuffer.c_str() << Log::endl;
            }

            Log::info() << "Compile DXC..." << Log::endl;

            // compile
            HR( compiler->Compile( &sourceCode, args->GetArguments(), args->GetCount(), includeHandler.ptr, IID_PPV_ARGS( &result.ptr ) ) );
        }

        compilerJobData->result = 0;

        // check status
        HRESULT status;
        HR( result->GetStatus( &status ) );

        if( SUCCEEDED( status ) )
        {
            Log::info() << " ...Success" << Log::endl;

            compilerJobData->result = 1;

            // errors
            if( result->HasOutput( DXC_OUT_ERRORS ) )
            {
                Ptr<IDxcBlobUtf8> errorMsgs;
                HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                if( errorMsgs.ptr && errorMsgs->GetStringLength() )
                {
                    Log::error() << errorMsgs->GetStringPointer() << Log::endl;
                }
            }

            // time report
            if( result->HasOutput( DXC_OUT_TIME_REPORT ) )
            {
                Ptr<IDxcBlobUtf8> timeReport;
                HR( result->GetOutput( DXC_OUT_TIME_REPORT, IID_PPV_ARGS( &timeReport.ptr ), nullptr ) );

                if( timeReport.ptr && timeReport->GetStringLength() )
                {
                    Log::info()  << "[REPORT] " << timeReport->GetStringPointer() << Log::endl;
                }
            }

            // time trace
            if( result->HasOutput( DXC_OUT_TIME_TRACE ) )
            {
                Ptr<IDxcBlobUtf8> timeTrace;
                HR( result->GetOutput( DXC_OUT_TIME_TRACE, IID_PPV_ARGS( &timeTrace.ptr ), nullptr ) );

                if( timeTrace.ptr && timeTrace->GetStringLength() )
                {
                    Log::info() << "[TRACE] " << timeTrace->GetStringPointer() << Log::endl;
                }
            }

            // text
            if( result->HasOutput( DXC_OUT_TEXT ) )
            {
                Ptr<IDxcBlobUtf8> text;
                HR( result->GetOutput( DXC_OUT_TEXT, IID_PPV_ARGS( &text.ptr ), nullptr ) );

                if( text.ptr && text->GetStringLength() )
                {
                    Log::info() << "[TEXT] " << text->GetStringPointer() << Log::endl;
                }
            }

            // remarks
            if( result->HasOutput( DXC_OUT_REMARKS ) )
            {
                Ptr<IDxcBlobUtf8> remarks;
                HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                if( remarks.ptr && remarks->GetStringLength() )
                {
                    Log::info() << "[REMARKS] " << remarks->GetStringPointer() << Log::endl;
                }
            }

            // root signature
            if( result->HasOutput( DXC_OUT_ROOT_SIGNATURE ) )
            {
                Ptr<IDxcBlobUtf8> rootSignature;
                HR( result->GetOutput( DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS( &rootSignature.ptr ), nullptr ) );

                if( rootSignature.ptr && rootSignature->GetBufferSize() )
                {
                    Log::info() << "[ROOTSIG] " << rootSignature->GetStringPointer() << Log::endl;
                }
            }

            // hlsl
            if( result->HasOutput( DXC_OUT_HLSL ) )
            {
                Ptr<IDxcBlobUtf8> hlsl;
                HR( result->GetOutput( DXC_OUT_HLSL, IID_PPV_ARGS( &hlsl.ptr ), nullptr ) );

                if( hlsl.ptr && hlsl->GetStringLength() )
                {
                    Log::info() << "[HSLS] " <<  hlsl->GetStringPointer() << Log::endl;
                }
            }

            // disassembly
            if( result->HasOutput( DXC_OUT_DISASSEMBLY ) )
            {
                Ptr<IDxcBlobUtf8> disassembly;
                HR( result->GetOutput( DXC_OUT_DISASSEMBLY, IID_PPV_ARGS( &disassembly.ptr ), nullptr ) );

                if( disassembly.ptr && disassembly->GetStringLength() )
                {
                    Log::info() << "[DISASSEMBLY] " << disassembly->GetStringPointer() << Log::endl;
                }
            }

            // disassembly
            if( result->HasOutput( DXC_OUT_EXTRA_OUTPUTS ) )
            {
                Ptr<IDxcExtraOutputs> extraOutput;
                HR( result->GetOutput( DXC_OUT_EXTRA_OUTPUTS, IID_PPV_ARGS( &extraOutput.ptr ), nullptr ) );

                if( extraOutput.ptr && extraOutput->GetOutputCount() )
                {
                    Log::info() << "[EXTRA] " << extraOutput->GetOutputCount() << Log::endl;
                }
            }

            //
            //
            //

            struct ShaderOutputHeader
            {
                zp_uint32_t id;
                zp_uint32_t version;
                zp_uint32_t shaderFeatureCount;
                zp_hash64_t shaderFeatureHash;
                zp_uint64_t pdbOffset;
                zp_uint64_t reflectionOffset;
                zp_uint64_t shaderDataOffset;
                zp_hash128_t hash;
            };

            struct ShaderOutputShaderFeature
            {
                FixedString32 name;
            };

            struct ShaderOutputPDBHeader
            {
                FixedString64 filePath;
                zp_uint64_t size;
            };

            ShaderOutputHeader shaderOutputHeader {
                .id = zp_make_cc4( "ZPSH" ),
                .version = 0,
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

                    const ShaderOutputShaderFeature shaderFeature {
                        .name = FixedString32( featureDefine.c_str(), featureDefine.length() )
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
                    char pdbFilePath[ 64 ];
                    int pathLength = ::WideCharToMultiByte( CP_UTF8, 0, pdbPathFromCompiler->GetStringPointer(), -1, pdbFilePath, ZP_ARRAY_SIZE( pdbFilePath ), nullptr, nullptr );

                    const ShaderOutputPDBHeader pdbHeader {
                        .filePath = FixedString64( pdbFilePath ),
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
                            ZP_ASSERT( resourceIndex != zp::npos );

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
                            .resourceCount = zp_uint8_t( resources.length() ),
                            .elementCount = zp_uint8_t( elements.length() ),
                            .inputCount = zp_uint8_t( inputs.length() ),
                            .outputCount = zp_uint8_t( outputs.length() ),
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

                    const zp_size_t compressedSize = zp_lzf_compress( shaderObjPtr, 0, shaderObjSize, compressedShaderBinary.ptr, 0 );
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
            char hashStr[ 32 + 1 ];
            zp_try_hash128_to_string( compilerJobData->resultShaderHash, hashStr );

            char featureHashStr[ 16 + 1 ];
            zp_try_hash64_to_string( compilerJobData->shaderFeature.shaderFeatureHash, featureHashStr );

            char currentDir[ 512 ];
            String currentDirPath = Platform::GetCurrentDirStr( currentDir );

            zp_bool_t r;

            FilePath dstFilePath = currentDirPath / "Library";

            r = Platform::CreateDirectory( dstFilePath.c_str() );
            ZP_ASSERT_MSG_ARGS( r, "dir: %s", dstFilePath.c_str() );

            dstFilePath / "ShaderCache";

            r = Platform::CreateDirectory( dstFilePath.c_str() );
            ZP_ASSERT_MSG_ARGS( r, "dir: %s", dstFilePath.c_str() );

            const char* srcFileName = zp_strrchr( task->srcFile.c_str(), Platform::PathSep ) + 1;

            dstFilePath / srcFileName;

            r = Platform::CreateDirectory( dstFilePath.c_str() );
            ZP_ASSERT_MSG_ARGS( r, "dir: %s", dstFilePath.c_str() );

            MutableFixedString128 dstFileName;
            dstFileName.append( hashStr );
            dstFileName.append( '.' );
            dstFileName.append( featureHashStr );
            dstFileName.append( '.' );
            dstFileName.append( kShaderTypes[ compilerJobData->shaderProgram ] );

            dstFilePath / dstFileName;

            // store out resulting path
            //compilerJobData->resultShaderFilePath = dstFilePath;
            compilerJobData->resultShaderFeatureHash = compilerJobData->shaderFeature.shaderFeatureHash;

            FileHandle dstFileHandle = Platform::OpenFileHandle( dstFilePath.c_str(), ZP_OPEN_FILE_MODE_WRITE );
            Platform::WriteFile( dstFileHandle, shaderStreamMemory.ptr, shaderStreamMemory.size );
            Platform::CloseFileHandle( dstFileHandle );
        }
        else
        {
            Log::error() << " ...Failed" << Log::endl;

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
