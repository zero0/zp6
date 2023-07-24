//
// Created by phosg on 7/23/2023.
//

#ifndef ZP_SHADERCOMPILER_H
#define ZP_SHADERCOMPILER_H

#include "AssetCompiler.h"

namespace zp
{
    namespace ShaderCompiler
    {
        void ShaderCompilerExecute( AssetCompilerTask* task );

        void* ShaderCompilerCreateTaskMemory( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine );

        void ShaderCompilerDestroyTaskMemory( MemoryLabel memoryLabel, void* ptr );
    }
}

#endif //ZP_SHADERCOMPILER_H
