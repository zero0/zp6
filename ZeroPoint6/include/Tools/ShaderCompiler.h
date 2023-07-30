//
// Created by phosg on 7/23/2023.
//

#ifndef ZP_SHADERCOMPILER_H
#define ZP_SHADERCOMPILER_H

#include "AssetCompiler.h"

namespace zp::ShaderCompiler
{
    void ShaderCompilerExecuteJob( const JobHandle& parentJob, AssetCompilerTask* task );

    void ShaderCompilerExecute( AssetCompilerTask* task );

    Memory ShaderCompilerCreateTaskMemory( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine );

    void ShaderCompilerDestroyTaskMemory( MemoryLabel memoryLabel, Memory memory );
}

#endif //ZP_SHADERCOMPILER_H
