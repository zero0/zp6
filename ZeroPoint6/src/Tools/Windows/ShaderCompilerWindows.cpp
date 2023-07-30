//
// Created by phosg on 7/20/2023.
//

#include <Windows.h>

#include "Core/Allocator.h"
#include "Core/Data.h"
#include "Core/Job.h"
#include "Core/Vector.h"
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
#define CROSS_PLATFORM_UUIDOF(interface, spec ) \
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

        ~Ptr()
        {
            if( ptr )
            {
                ptr->Release();
            }
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
        SHADER_PROGRAM_SUPPORTED_TYPE_NONE = 0,
        SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX = 1 << SHADER_PROGRAM_TYPE_VERTEX,
        SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT = 1 << SHADER_PROGRAM_TYPE_FRAGMENT,
        SHADER_PROGRAM_SUPPORTED_TYPE_GEOMETRY = 1 << SHADER_PROGRAM_TYPE_GEOMETRY,
        SHADER_PROGRAM_SUPPORTED_TYPE_TESSELATION = 1 << SHADER_PROGRAM_TYPE_TESSELATION,
        SHADER_PROGRAM_SUPPORTED_TYPE_HULL = 1 << SHADER_PROGRAM_TYPE_HULL,
        SHADER_PROGRAM_SUPPORTED_TYPE_DOMAIN = 1 << SHADER_PROGRAM_TYPE_DOMAIN,
        SHADER_PROGRAM_SUPPORTED_TYPE_TASK = 1 << SHADER_PROGRAM_TYPE_TASK,
        SHADER_PROGRAM_SUPPORTED_TYPE_MESH = 1 << SHADER_PROGRAM_TYPE_MESH,
        SHADER_PROGRAM_SUPPORTED_TYPE_COMPUTE = 1 << SHADER_PROGRAM_TYPE_COMPUTE,
    };

    LPCWSTR kShaderTypes[] {
        L"vs",
        L"ps",
        L"gs",
        L"ts",
        L"hs",
        L"ds",
        L"as",
        L"ms",
        L"cs",
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
        kEntryPointSize = 128
    };

    struct ShaderTaskData
    {
        Memory shaderSource;

        FixedString<kEntryPointSize> entryPoints[ShaderProgramType_Count];

        zp_uint32_t shaderCompilerSupportedTypes;
        ShaderModelType shaderModel;

        ShaderAPI shaderAPI;
        ShaderOutput shaderOutput;

        ZP_BOOL32( debug );
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
        explicit LocalMalloc( zp::MemoryLabel memoryLabel )
            : memoryLabel( memoryLabel )
        {
        }

        void* Alloc( SIZE_T cb ) override
        {
            return ZP_MALLOC_( memoryLabel, cb );
        }

        void* Realloc( void* pv, SIZE_T cb ) override
        {
            return ZP_REALLOC( memoryLabel, pv, cb );
        }

        void Free( void* pv ) override
        {
            ZP_FREE_( memoryLabel, pv );
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
        const zp::MemoryLabel memoryLabel;
    };

    //
    //
    //

    void PreprocessShader( const JobHandle& parentJob, ShaderTaskData* shaderTask )
    {

    }
}

//
//
//

namespace zp::ShaderCompiler
{
    struct CompileJob
    {

    };

    void ShaderCompilerExecuteJob( const JobHandle& parentJob, AssetCompilerTask* task )
    {
        ShaderCompilerExecute( task );
    }

    void ShaderCompilerExecute( AssetCompilerTask* task )
    {
        ShaderTaskData* data = reinterpret_cast<ShaderTaskData*>(task->taskMemory.ptr);

        DxcBuffer sourceCode {
            .Ptr = data->shaderSource.ptr,
            .Size = data->shaderSource.size,
            .Encoding = 0,
        };

        HRESULT hr;

        AllocString srcFile = task->srcFile;
        AllocString dstFile = task->dstFile;

        LocalMalloc malloc( 0 );

        for( zp_uint32_t shaderType = 0; shaderType < ShaderProgramType_Count; ++shaderType )
        {
            if( data->shaderCompilerSupportedTypes & ( 1 << shaderType ) )
            {
                ShaderModel sm = kShaderModelTypes[ data->shaderModel ];

                auto start = zp_strrchr( srcFile.c_str(), '\\' ) + 1;
                auto end = zp_strrchr( srcFile.c_str(), '.' );

                char entryPoint[kEntryPointSize];
                zp_snprintf( entryPoint, "%.*s%s", end - start, start, kShaderEntryName[ shaderType ] );

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
                argsBuffer.appendFormat( "\n" );

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

                            zp_handle_t pdbFileHandle = GetPlatform()->OpenFileHandle( pdbFilePath, ZP_OPEN_FILE_MODE_WRITE );
                            GetPlatform()->WriteFile( pdbFileHandle, pdbData->GetBufferPointer(), pdbData->GetBufferSize() );
                            GetPlatform()->CloseFileHandle( pdbFileHandle );

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

                        zp_handle_t dstFileHandle = GetPlatform()->OpenFileHandle( dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
                        GetPlatform()->WriteFile( dstFileHandle, shaderObj->GetBufferPointer(), shaderObj->GetBufferSize() );
                        GetPlatform()->CloseFileHandle( dstFileHandle );

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
#if 0
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
#endif
    }

    Memory ShaderCompilerCreateTaskMemory( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine )
    {
        ShaderTaskData* data = nullptr;

        if( GetPlatform()->FileExists( inFile.c_str() ) )
        {
            data = ZP_MALLOC_T( memoryLabel, ShaderTaskData );

            data->debug = false;

            zp_handle_t inFileHandle = GetPlatform()->OpenFileHandle( inFile.c_str(), ZP_OPEN_FILE_MODE_READ );
            const zp_size_t inFileSize = GetPlatform()->GetFileSize( inFileHandle );
            void* inFileData = ZP_MALLOC_( memoryLabel, inFileSize );

            zp_size_t inFileReadSize = GetPlatform()->ReadFile( inFileHandle, inFileData, inFileSize );
            ZP_ASSERT( inFileSize == inFileReadSize );

            // TODO: move to helper functions
            Tokenizer lineTokenizer( static_cast<const char*>( inFileData ), inFileSize, "\r\n" );

            String line {};
            while( lineTokenizer.next( line ) )
            {
                if( zp_strnstr( line.c_str(), line.length, "#pragma " ) )
                {
                    Tokenizer pragmaTokenizer( line.c_str(), line.length, " " );

                    String pragma {};
                    while( pragmaTokenizer.next( pragma ) )
                    {
                        if( zp_strcmp( "vertex", pragma.c_str(), pragma.length ) == 0 )
                        {
                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_VERTEX ] = pragmaOp;
                                zp_printfln( "Vertex Entry - %.*s", pragmaOp.length, pragmaOp.str );
                            }
                        }
                        else if( zp_strcmp( "fragment", pragma.c_str(), pragma.length ) == 0 )
                        {
                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                data->entryPoints[ SHADER_PROGRAM_TYPE_FRAGMENT ] = pragmaOp;
                                zp_printfln( "Fragment Entry - %.*s", pragmaOp.length, pragmaOp.str );
                            }
                        }
                        else if( zp_strcmp( "enable_debug", pragma.c_str(), pragma.length ) == 0 )
                        {
                            data->debug = true;
                            zp_printfln( "Use Debug - true" );
                        }
                        else if( zp_strcmp( "target", pragma.c_str(), pragma.length ) == 0 )
                        {
                            String pragmaOp {};
                            if( pragmaTokenizer.next( pragmaOp ) )
                            {
                                zp_printfln( "Target - %.*s", pragmaOp.length, pragmaOp.str );
                            }
                        }
                            // TODO: have dxc precompile hlsl and then parse pragmas from that to support pragmas in included files, then compile variants from the preprocessed
                        else if( zp_strnstr( pragma.c_str(), pragma.length, "shader_feature" ) != nullptr )
                        {
                            zp_uint32_t programSupportedType = SHADER_PROGRAM_SUPPORTED_TYPE_NONE;

                            if( zp_strcmp( "shader_feature_vertex", pragma.c_str(), pragma.length ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                            }
                            else if( zp_strcmp( "shader_feature_fragment", pragma.c_str(), pragma.length ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            }
                            else if( zp_strcmp( "shader_feature", pragma.c_str(), pragma.length ) == 0 )
                            {
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX;
                                programSupportedType |= SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
                            }

                            if( programSupportedType != 0 )
                            {
                                MutableFixedString<4> pp;
                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX )
                                {
                                    pp.appendFormat( "V" );
                                }
                                if( programSupportedType & SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT )
                                {
                                    pp.appendFormat( "F" );
                                }

                                zp_printfln( "Shader Features %s:", pp.c_str() );

                                String pragmaOp {};
                                while( pragmaTokenizer.next( pragmaOp ) )
                                {
                                    zp_printfln( "  - %.*s", pragmaOp.length, pragmaOp.str );
                                }
                            }
                            break;
                        }
                        else if( zp_strnstr( pragma.c_str(), pragma.length, "invalid_shader_feature" ) != nullptr )
                        {
                            zp_printfln( "Invalid Shader Features:" );

                            String pragmaOp {};
                            while( pragmaTokenizer.next( pragmaOp ) )
                            {
                                zp_printfln( "  - %.*s", pragmaOp.length, pragmaOp.str );
                            }
                        }
                    }
                }
            }

            data->shaderSource = { .ptr = inFileData, .size =inFileSize };

            data->shaderCompilerSupportedTypes = SHADER_PROGRAM_SUPPORTED_TYPE_VERTEX | SHADER_PROGRAM_SUPPORTED_TYPE_FRAGMENT;
            data->shaderModel = SHADER_MODEL_TYPE_6_0;

            data->shaderAPI = SHADER_API_D3D;
            data->shaderOutput = SHADER_OUTPUT_SPIRV;

            GetPlatform()->CloseFileHandle( inFileHandle );
        }

        return { .ptr = data, .size = data == nullptr ? 0 : sizeof( ShaderTaskData ) };
    }

    void ShaderCompilerDestroyTaskMemory( MemoryLabel memoryLabel, Memory memory )
    {
        if( memory.ptr )
        {
            ShaderTaskData* data = reinterpret_cast<ShaderTaskData*>(memory.ptr);

            ZP_SAFE_FREE_LABEL( memoryLabel, data->shaderSource.ptr );

            ZP_FREE_( memoryLabel, memory.ptr );
        }
    }
}
