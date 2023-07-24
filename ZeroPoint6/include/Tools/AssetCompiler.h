//
// Created by phosg on 7/16/2023.
//

#ifndef ZP_ASSETCOMPILER_H
#define ZP_ASSETCOMPILER_H

#include "Core/Common.h"
#include "Core/Macros.h"
#include "Core/Types.h"
#include "Core/Allocator.h"
#include "Core/String.h"
#include "Core/Map.h"
#include "Core/CommandLine.h"

namespace zp
{
    enum AssetCompilerExecuteType
    {

    };

    struct AssetCompilerTask;

    typedef void (* AssetProcessorExecuteFunc)( AssetCompilerTask* task );

    typedef void* (* AssetProcessorCreateTaskMemoryFunc)( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine );

    typedef void (* AssetProcessorDeleteTaskMemoryFunc)( MemoryLabel memoryLabel, void* ptr );

    struct AssetCompilerTask
    {
        AllocString srcFile;
        AllocString dstFile;

        AssetProcessorExecuteFunc exec;
        AssetProcessorDeleteTaskMemoryFunc deleteTaskMemory;

        void* ptr;
        MemoryLabel memoryLabel;
    };

    struct AssetCompilerProcessor
    {
        AssetProcessorCreateTaskMemoryFunc createTaskFunc;
        AssetProcessorDeleteTaskMemoryFunc deleteTaskFunc;
        AssetProcessorExecuteFunc executeFunc;
    };

    class AssetCompiler
    {
    public:
        explicit AssetCompiler( MemoryLabel memoryLabel );

        ~AssetCompiler();

        void registerFileExtension( const String& ext, const AssetCompilerProcessor& assetProcessor );

        [[nodiscard]] const AssetCompilerProcessor* getCompilerProcessor( const String& ext ) const;

    private:
        Map<zp_hash64_t, AssetCompilerProcessor> m_assetProcessors;

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_ASSETCOMPILER_H
