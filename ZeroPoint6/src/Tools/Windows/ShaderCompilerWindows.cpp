//
// Created by phosg on 7/20/2023.
//

#include <Windows.h>

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
#define CROSS_PLATFORM_UUIDOF(interface, spec)  \
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

#define HR( r )   hr = (r)

#pragma comment(lib, "dxcompiler.lib")

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