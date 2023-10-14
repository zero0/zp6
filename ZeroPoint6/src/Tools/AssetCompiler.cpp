//
// Created by phosg on 7/16/2023.
//

#include "Tools/AssetCompiler.h"

using namespace zp;

AssetCompiler::AssetCompiler( MemoryLabel memoryLabel )
    : m_assetProcessors( memoryLabel, 16, memoryLabel )
    , memoryLabel( memoryLabel )
{
}

AssetCompiler::~AssetCompiler()
{
}

void AssetCompiler::registerFileExtension( const String& ext, const AssetCompilerProcessor& assetProcessor )
{
    m_assetProcessors.set( ext, assetProcessor );
}

const AssetCompilerProcessor* AssetCompiler::getCompilerProcessor( const String& ext ) const
{
    AssetCompilerProcessor* assetCompilerProcessor;
    return m_assetProcessors.tryGet( ext, &assetCompilerProcessor ) ? assetCompilerProcessor : nullptr;
}
