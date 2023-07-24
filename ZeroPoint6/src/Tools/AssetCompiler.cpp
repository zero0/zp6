//
// Created by phosg on 7/16/2023.
//

#include "Tools/AssetCompiler.h"

using namespace zp;

AssetCompiler::AssetCompiler( zp::MemoryLabel memoryLabel )
    : m_assetProcessors( 16, memoryLabel )
    , memoryLabel( memoryLabel )
{
}

AssetCompiler::~AssetCompiler()
{
}

void AssetCompiler::registerFileExtension( const String& ext, const AssetCompilerProcessor& assetProcessor )
{
    const zp_hash64_t hash = zp_fnv64_1a( ext.str, ext.length );
    m_assetProcessors.set( hash, assetProcessor );
}

const AssetCompilerProcessor* AssetCompiler::getCompilerProcessor( const String& ext ) const
{
    const zp_hash64_t hash = zp_fnv64_1a( ext.str, ext.length );
    AssetCompilerProcessor* assetCompilerProcessor = {};
    m_assetProcessors.get( hash, &assetCompilerProcessor );
    return assetCompilerProcessor;
}