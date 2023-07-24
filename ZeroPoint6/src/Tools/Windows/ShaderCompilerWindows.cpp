//
// Created by phosg on 7/20/2023.
//

#include <Windows.h>

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
        (int)spec_to_int32(   (spec)+0),        \
        (short)spec_to_int16( (spec)+9),        \
        (short)spec_to_int16( (spec)+14),       \
        (char)spec_to_int8(   (spec)+19),       \
        (char)spec_to_int8(   (spec)+21),       \
        (char)spec_to_int8(   (spec)+24),       \
        (char)spec_to_int8(   (spec)+26),       \
        (char)spec_to_int8(   (spec)+28),       \
        (char)spec_to_int8(   (spec)+30),       \
        (char)spec_to_int8(   (spec)+32),       \
        (char)spec_to_int8(   (spec)+34)        \
        );

#define DXC_API_IMPORT __attribute__ ((visibility ("default")))

#include <dxc/dxcapi.h>
#include <d3d12shader.h>

#define HR( r )     do { hr = (r); ZP_ASSERT_MSG( SUCCEEDED(hr), #r ); } while( false )

namespace
{
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

    enum ShaderCompilerType
    {
        SHADER_COMPILER_TYPE_VERTEX,
        SHADER_COMPILER_TYPE_FRAGMENT,
        SHADER_COMPILER_TYPE_GEOMETRY,
        SHADER_COMPILER_TYPE_TESSELATION,
        SHADER_COMPILER_TYPE_HULL,
        SHADER_COMPILER_TYPE_DOMAIN,
        SHADER_COMPILER_TYPE_TASK,
        SHADER_COMPILER_TYPE_MESH,
        SHADER_COMPILER_TYPE_COMPUTE,

        ShaderCompilerType_Count,
    };

    enum ShaderCompilerSupportedType
    {
        SHADER_COMPILER_SUPPORTED_TYPE_VERTEX = 1 << SHADER_COMPILER_TYPE_VERTEX,
        SHADER_COMPILER_SUPPORTED_TYPE_FRAGMENT = 1 << SHADER_COMPILER_TYPE_FRAGMENT,
        SHADER_COMPILER_SUPPORTED_TYPE_GEOMETRY = 1 << SHADER_COMPILER_TYPE_GEOMETRY,
        SHADER_COMPILER_SUPPORTED_TYPE_TESSELATION = 1 << SHADER_COMPILER_TYPE_TESSELATION,
        SHADER_COMPILER_SUPPORTED_TYPE_HULL = 1 << SHADER_COMPILER_TYPE_HULL,
        SHADER_COMPILER_SUPPORTED_TYPE_DOMAIN = 1 << SHADER_COMPILER_TYPE_DOMAIN,
        SHADER_COMPILER_SUPPORTED_TYPE_TASK = 1 << SHADER_COMPILER_TYPE_TASK,
        SHADER_COMPILER_SUPPORTED_TYPE_MESH = 1 << SHADER_COMPILER_TYPE_MESH,
        SHADER_COMPILER_SUPPORTED_TYPE_COMPUTE = 1 << SHADER_COMPILER_TYPE_COMPUTE,
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
    ZP_STATIC_ASSERT( ShaderCompilerType_Count == ZP_ARRAY_SIZE( kShaderTypes ) );

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

    struct ShaderTaskData
    {
        void* shaderSourcePtr;
        zp_size_t shaderSourceSize;

        zp_uint32_t shaderCompilerSupportedTypes;
        ShaderModelType shaderModel;

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
    ZP_STATIC_ASSERT( ShaderCompilerType_Count == ZP_ARRAY_SIZE( kShaderEntryName ) );
}

namespace zp
{
    namespace ShaderCompiler
    {
        void ShaderCompilerExecute( AssetCompilerTask* task )
        {
            ShaderTaskData* data = reinterpret_cast<ShaderTaskData*>(task->ptr);

            DxcBuffer sourceCode {
                .Ptr = data->shaderSourcePtr,
                .Size = data->shaderSourceSize,
                .Encoding = 0,
            };

            HRESULT hr;

            for( zp_uint32_t shaderType = 0; shaderType < ShaderCompilerType_Count; ++shaderType )
            {
                if( data->shaderCompilerSupportedTypes & ( 1 << shaderType ) )
                {
                    ShaderModel sm = kShaderModelTypes[ data->shaderModel ];

                    auto start = zp_strrchr( task->srcFile.c_str(), '\\' ) + 1;
                    auto end = zp_strrchr( task->srcFile.c_str(), '.' );

                    char entryPoint[128];
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

                    Vector<LPCWSTR> arguments( 24, 0 );
                    if( data->debug )
                    {
                        arguments.pushBack( DXC_ARG_DEBUG );
                        arguments.pushBack( DXC_ARG_SKIP_OPTIMIZATIONS );
                    }
                    else
                    {
                        arguments.pushBack( DXC_ARG_OPTIMIZATION_LEVEL3 );
                        arguments.pushBack( DXC_ARG_WARNINGS_ARE_ERRORS );
                        arguments.pushBack( L"-remove-unused-functions" );
                        arguments.pushBack( L"-remove-unused-globals" );
                    }

                    arguments.pushBack( DXC_ARG_DEBUG_NAME_FOR_BINARY );

                    arguments.pushBack( L"-ftime-report" );
                    arguments.pushBack( L"-ftime-trace" );

                    arguments.pushBack( L"-Qstrip_debug" );
                    //arguments.pushBack( L"-Qstrip_priv" );
                    //arguments.pushBack( L"-Qstrip_reflect" );
                    //arguments.pushBack( L"-Qstrip_rootsignature" );

                    arguments.pushBack( L"-I" );
                    arguments.pushBack( L"Assets/ShaderLibrary/" );

                    arguments.pushBack( L"-fspv-entrypoint-name=main" );
                    arguments.pushBack( L"-fspv-reflect" );

                    arguments.pushBack( L"-spirv" );

                    Vector<DxcDefine> defines( 4, 0 );
                    defines.pushBack( { .Name= L"SHADER_API_D3D", .Value = L"1" } );
                    //defines.pushBack( { .Name= L"SHADER_API_VULKAN", .Value = L"1" } );
                    if( data->debug )
                    {
                        defines.pushBack( { .Name= L"DEBUG", .Value = L"1" } );
                    }

                    Ptr<IDxcUtils> utils;
                    HR( DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &utils.ptr ) ) );

                    Ptr<IDxcIncludeHandler> includeHandler;
                    HR( utils->CreateDefaultIncludeHandler( &includeHandler.ptr ) );

                    Ptr<IDxcCompiler3> compiler;
                    HR( DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler.ptr ) ) );

                    WCHAR srcFileName[512];
                    ::MultiByteToWideChar( CP_UTF8, 0, task->srcFile.c_str(), -1, srcFileName, ZP_ARRAY_SIZE( srcFileName ) );

                    WCHAR entryPointName[128];
                    ::MultiByteToWideChar( CP_UTF8, 0, entryPoint, -1, entryPointName, ZP_ARRAY_SIZE( entryPointName ) );

                    Ptr<IDxcCompilerArgs> args;
                    utils->BuildArguments( srcFileName, entryPointName, targetProfiler, arguments.begin(), arguments.size(), defines.begin(), defines.size(), &args.ptr );

                    zp_printf( "Args: " );
                    auto f = args->GetArguments();
                    for( zp_size_t i = 0; i < args->GetCount(); ++i )
                    {
                        zp_printf( "%ls ", *f );
                        ++f;
                    }
                    zp_printf( "\n" );

                    Ptr<IDxcResult> result;
                    HR( compiler->Compile( &sourceCode, args->GetArguments(), args->GetCount(), includeHandler.ptr, IID_PPV_ARGS( &result.ptr ) ) );

                    HRESULT status;
                    HR( result->GetStatus( &status ) );

                    if( SUCCEEDED( status ) )
                    {
                        if( result->HasOutput( DXC_OUT_ERRORS ) )
                        {
                            Ptr<IDxcBlobUtf8> errorMsgs;
                            HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                            if( errorMsgs.ptr && errorMsgs->GetStringLength() )
                            {
                                zp_printfln( ZP_CC_N( RED, DEFAULT ) "[ERROR]" ZP_CC_RESET " %s", errorMsgs->GetStringPointer() );
                            }
                        }

                        if( result->HasOutput( DXC_OUT_TIME_REPORT ) )
                        {
                            Ptr<IDxcBlobUtf8> timeReport;
                            HR( result->GetOutput( DXC_OUT_TIME_REPORT, IID_PPV_ARGS( &timeReport.ptr ), nullptr ) );

                            if( timeReport.ptr && timeReport->GetStringLength() )
                            {
                                zp_printfln( "[INFO] " " %s", timeReport->GetStringPointer() );
                            }
                        }

                        if( result->HasOutput( DXC_OUT_TIME_TRACE ) )
                        {
                            Ptr<IDxcBlobUtf8> timeTrace;
                            HR( result->GetOutput( DXC_OUT_TIME_TRACE, IID_PPV_ARGS( &timeTrace.ptr ), nullptr ) );

                            if( timeTrace.ptr && timeTrace->GetStringLength() )
                            {
                                zp_printfln( "[INFO] " " %s", timeTrace->GetStringPointer() );
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
                                char pdbFilePath[512];
                                ::WideCharToMultiByte( CP_UTF8, 0, pdbPathFromCompiler->GetStringPointer(), -1, pdbFilePath, ZP_ARRAY_SIZE( pdbFilePath ), nullptr, nullptr );

                                zp_handle_t pdbFileHandle = GetPlatform()->OpenFileHandle( pdbFilePath, ZP_OPEN_FILE_MODE_WRITE );
                                GetPlatform()->WriteFile( pdbFileHandle, pdbData->GetBufferPointer(), pdbData->GetBufferSize() );
                                GetPlatform()->CloseFileHandle( pdbFileHandle );
                            }
                        }

                        // reflection
                        if( result->HasOutput( DXC_OUT_REFLECTION ) )
                        {
                            Ptr<IDxcBlob> reflection;
                            HR( result->GetOutput( DXC_OUT_REFLECTION, IID_PPV_ARGS( &reflection.ptr ), nullptr ) );

                            if( reflection.ptr )
                            {
                                // save to file also
                                DxcBuffer reflectionData {
                                    .Ptr = reflection->GetBufferPointer(),
                                    .Size = reflection->GetBufferSize(),
                                    .Encoding = 0
                                };

                                Ptr<ID3D12ShaderReflection> shaderReflection;
                                HR( utils->CreateReflection( &reflectionData, IID_PPV_ARGS( &shaderReflection.ptr ) ) );
                            }
                        }

                        if( result->HasOutput( DXC_OUT_SHADER_HASH ) )
                        {
                            Ptr<IDxcBlob> shaderHash;
                            HR( result->GetOutput( DXC_OUT_SHADER_HASH, IID_PPV_ARGS( &shaderHash.ptr ), nullptr ) );

                            if( shaderHash.ptr && shaderHash->GetBufferSize() )
                            {
                                zp_printfln( "[INFO] " " Hash: %p", shaderHash->GetBufferPointer() );
                            }
                        }

                        if( result->HasOutput( DXC_OUT_REMARKS ) )
                        {
                            Ptr<IDxcBlobUtf8> remarks;
                            HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                            if( remarks.ptr && remarks->GetStringLength() )
                            {
                                zp_printfln( "[INFO] " " %s", remarks->GetStringPointer() );
                            }
                        }

                        // shader byte code
                        ZP_ASSERT( result->HasOutput( DXC_OUT_OBJECT ) );

                        Ptr<IDxcBlob> shaderObj;
                        HR( result->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &shaderObj.ptr ), nullptr ) );

                        zp_handle_t dstFileHandle = GetPlatform()->OpenFileHandle( task->dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
                        GetPlatform()->WriteFile( dstFileHandle, shaderObj->GetBufferPointer(), shaderObj->GetBufferSize() );
                        GetPlatform()->CloseFileHandle( dstFileHandle );
                    }
                    else
                    {
                        if( result->HasOutput( DXC_OUT_ERRORS ) )
                        {
                            Ptr<IDxcBlobUtf8> errorMsgs;
                            HR( result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs.ptr ), nullptr ) );

                            if( errorMsgs->GetStringLength() )
                            {
                                zp_printfln( ZP_CC_N( RED, DEFAULT ) "[ERROR]" ZP_CC_RESET " %s", errorMsgs->GetStringPointer() );
                            }
                        }

                        if( result->HasOutput( DXC_OUT_REMARKS ) )
                        {
                            Ptr<IDxcBlobUtf8> remarks;
                            HR( result->GetOutput( DXC_OUT_REMARKS, IID_PPV_ARGS( &remarks.ptr ), nullptr ) );

                            if( remarks.ptr && remarks->GetStringLength() )
                            {
                                zp_printfln( "[INFO] " " %s", remarks->GetStringPointer() );
                            }
                        }
                    }
                }
            }
        }

        void* ShaderCompilerCreateTaskMemory( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine )
        {
            ShaderTaskData* data = nullptr;

            if( GetPlatform()->FileExists( inFile.c_str() ) )
            {
                data = ZP_MALLOC_T( memoryLabel, ShaderTaskData );

                zp_handle_t inFileHandle = GetPlatform()->OpenFileHandle( inFile.c_str(), ZP_OPEN_FILE_MODE_READ );
                const zp_size_t inFileSize = GetPlatform()->GetFileSize( inFileHandle );
                void* inFileData = ZP_MALLOC_( memoryLabel, inFileSize );

                zp_size_t inFileReadSize = GetPlatform()->ReadFile( inFileHandle, inFileData, inFileSize );
                ZP_ASSERT( inFileSize == inFileReadSize );

                data->shaderSourcePtr = inFileData;
                data->shaderSourceSize = inFileSize;

                data->shaderCompilerSupportedTypes = SHADER_COMPILER_SUPPORTED_TYPE_VERTEX | SHADER_COMPILER_SUPPORTED_TYPE_FRAGMENT;
                data->shaderModel = SHADER_MODEL_TYPE_5_0;

                data->debug = true;

                GetPlatform()->CloseFileHandle( inFileHandle );
            }

            return data;
        }

        void ShaderCompilerDestroyTaskMemory( MemoryLabel memoryLabel, void* ptr )
        {
            ShaderTaskData* data = reinterpret_cast<ShaderTaskData*>(ptr);

            ZP_FREE_( memoryLabel, data->shaderSourcePtr );

            ZP_FREE_( memoryLabel, ptr );
        }
    }
}

void InitDxcCompiler()
{
    HRESULT hr;

    IDxcUtils* utils;
    hr = DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &utils ) );

    IDxcIncludeHandler* includeHandler;
    utils->CreateDefaultIncludeHandler( &includeHandler );

    IDxcCompiler3* compiler3;
    DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler3 ) );
}

void CompileShader()
{
    HRESULT hr;

    IDxcUtils* utils;
    hr = DxcCreateInstance( CLSID_DxcUtils, IID_PPV_ARGS( &utils ) );

    IDxcIncludeHandler* includeHandler;
    utils->CreateDefaultIncludeHandler( &includeHandler );

    IDxcCompiler3* compiler3;
    DxcCreateInstance( CLSID_DxcCompiler, IID_PPV_ARGS( &compiler3 ) );

    DxcBuffer* source;
    LPCWSTR* args;

    IDxcResult* result;
    compiler3->Compile( source, args, 0, includeHandler, IID_PPV_ARGS( &result ) );

    result->GetStatus( &hr );

    IDxcBlobUtf8* errorMsgs;
    result->GetOutput( DXC_OUT_ERRORS, IID_PPV_ARGS( &errorMsgs ), nullptr );

    if( FAILED( hr ) )
    {

    }

    // shader byte code
    IDxcBlob* shaderObj;
    result->GetOutput( DXC_OUT_OBJECT, IID_PPV_ARGS( &shaderObj ), nullptr );

    // pdb file and path
    IDxcBlob* pdbData;
    IDxcBlobUtf16* pdbPathFromCompiler;
    result->GetOutput( DXC_OUT_PDB, IID_PPV_ARGS( &pdbData ), &pdbPathFromCompiler );

    // reflection
    IDxcBlob* reflection;
    result->GetOutput( DXC_OUT_REFLECTION, IID_PPV_ARGS( &reflection ), nullptr );

    // save to file also
    DxcBuffer reflectionData {
        .Ptr = reflection->GetBufferPointer(),
        .Size = reflection->GetBufferSize(),
        .Encoding = 0
    };

    ID3D12ShaderReflection* shaderReflection;
    utils->CreateReflection( &reflectionData, IID_PPV_ARGS( &shaderReflection ) );

}