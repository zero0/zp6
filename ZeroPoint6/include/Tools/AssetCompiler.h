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
#include "Core/Job.h"

namespace zp
{
    enum AssetCompilerExecuteType
    {

    };

    struct AssetCompilerTask;

    typedef void (* AssetProcessorExecuteFunc)( AssetCompilerTask* task );

    typedef void (* AssetProcessorExecuteJobFunc)( const JobHandle& parentJob, AssetCompilerTask* task );

    typedef Memory (* AssetProcessorCreateTaskMemoryFunc)( MemoryLabel memoryLabel, const String& inFile, const String& outFile, const CommandLine& cmdLine );

    typedef void (* AssetProcessorDeleteTaskMemoryFunc)( MemoryLabel memoryLabel, Memory memory );

    struct AssetCompilerTask
    {
        AllocString srcFile;
        AllocString dstFile;

        AssetProcessorExecuteFunc exec;
        AssetProcessorExecuteJobFunc jobExec;
        AssetProcessorDeleteTaskMemoryFunc deleteTaskMemory;

        Memory taskMemory;
        MemoryLabel memoryLabel;

        static void Execute( const JobHandle& jobHandle, AssetCompilerTask* task );
    };

    struct AssetCompilerProcessor
    {
        AssetProcessorCreateTaskMemoryFunc createTaskFunc;
        AssetProcessorDeleteTaskMemoryFunc deleteTaskFunc;
        AssetProcessorExecuteFunc executeFunc;
        AssetProcessorExecuteJobFunc jobExecuteFunc;
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
