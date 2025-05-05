//
// Created by phosg on 4/20/2025.
//

#include "Core/Allocator.h"
#include "Core/Data.h"
#include "Core/Job.h"
#include "Core/Vector.h"
#include "Core/String.h"
#include "Platform/Platform.h"

#include "Tools/ShaderCompiler.h"

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
        for( zp_size_t s = 0; s < data->shaderFeatures.length(); ++s )
        {
            const ShaderFeature& shaderFeature = data->shaderFeatures[ s ];

            for( zp_uint32_t shaderProgram = 0; shaderProgram < ShaderProgramType_Count; ++shaderProgram )
            {
                const zp_uint32_t shaderProgramType = 1 << shaderProgram;

                // check shader feature is supported for the program type
                zp_bool_t shaderFeatureSupported = ( shaderFeature.shaderProgramSupportedType & shaderProgramType ) == shaderProgramType;

                // check for invalid shader features
                if( shaderFeatureSupported )
                {
                    for( const ShaderFeature& invalidShaderFeature : data->invalidShaderFeatures )
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
                    if( ( data->shaderCompilerSupportedTypes & shaderProgramType ) == shaderProgramType )
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

        JobHandle parentCompileJob {}; //= JobSystem::Start().schedule();

        // compile each job
        for( CompileJob& job : compileJobs )
        {
            if( useJobSystem )
            {
                // TODO: implement using new job system
                //JobSystem::Start( CompileShaderDXC, job, parentCompileJob ).schedule();
            }
            else
            {
                CompileShaderDXC( parentCompileJob, &job );
            }
        }

        JobSystem::ScheduleBatchJobs();
        JobSystem::Complete( parentCompileJob );

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

            FileHandle dstFileHandle = Platform::OpenFileHandle( task->dstFile.c_str(), ZP_OPEN_FILE_MODE_WRITE );
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

            FileHandle inFileHandle = Platform::OpenFileHandle( inFile.c_str(), ZP_OPEN_FILE_MODE_READ );
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
                                if( pragmaOp.length() == 3 && pragmaOp.str()[ 1 ] == '.' )
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
                                    zp_printfln( "  - %.*s", pragmaOp.length(), pragmaOp.c_str() );

                                    // build feature list, add new one if it doesn't exist
                                    zp_size_t index = data->allShaderFeatures.findIndexOf( [&pragmaOp]( const String& x )
                                    {
                                        return zp_strcmp( x, pragmaOp ) == 0;
                                    } );

                                    if( index == zp::npos )
                                    {
                                        index = data->allShaderFeatures.length();
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
                                zp_printfln( "  - %.*s", pragmaOp.length(), pragmaOp.c_str() );

                                // build feature list, add new one if it doesn't exist
                                zp_size_t index = data->allShaderFeatures.findIndexOf( [&pragmaOp]( const String& x )
                                {
                                    return zp_strcmp( x, pragmaOp ) == 0;
                                } );

                                if( index == zp::npos )
                                {
                                    index = data->allShaderFeatures.length();
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
