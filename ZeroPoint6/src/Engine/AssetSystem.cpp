//
//
//

#include "Core/Allocator.h"

#include "Platform/Platform.h"

#include "Engine/AssetSystem.h"
#include "Engine/MemoryLabels.h"

#include "Engine/TransformComponent.h"

namespace zp
{
    AssetSystem::AssetSystem( MemoryLabel memoryLabel )
        : m_virtualFileSystem( nullptr )
        , m_entityComponentManager( nullptr )
        , m_assetMemoryLabels {}
        , memoryLabel( memoryLabel )
    {
        for( MemoryLabel& assetMemoryLabel : m_assetMemoryLabels )
        {
            assetMemoryLabel = memoryLabel;
        }
    }

    AssetSystem::~AssetSystem()
    {
    }

    void AssetSystem::setup( EntityComponentManager* entityComponentManager )
    {
        m_entityComponentManager = entityComponentManager;
    }

    void AssetSystem::teardown()
    {
        m_entityComponentManager = nullptr;
    }

    JobHandle AssetSystem::process( EntityComponentManager* entityComponentManager, const JobHandle& inputHandle )
    {
        EntityQueryIterator iterator {};
        entityComponentManager->iterateEntities( {
            .notIncludedTags = entityComponentManager->getTagSignature<DestroyedTag>(),
            .requiredStructures = entityComponentManager->getComponentSignature<AssetReferenceCountComponentData>(),
        }, &iterator );

        while( entityComponentManager->next( &iterator ) )
        {
            AssetReferenceCountComponentData* data = iterator.getComponentData<AssetReferenceCountComponentData>();
            if( data && data->refCount == 0 )
            {
                iterator.addTag<DestroyedTag>();
            }
        }

        return inputHandle;
    }

    Entity AssetSystem::loadFileDirect( const char* filePath )
    {
        Entity entity = m_entityComponentManager->createEntity( {
            .structuralSignature = m_entityComponentManager->getComponentSignature<RawAssetComponentData>()
        } );

        RawAssetComponentData* data = m_entityComponentManager->getComponentData<RawAssetComponentData>( entity );
        *data = {
            .guid = {},
            .hash = {},
            .data = {}
        };

        struct LoadFileJob
        {
            RawAssetComponentData* componentData;
            const char* filePath;

            static void Execute( const JobHandle& parentJobHandle, const LoadFileJob* ptr )
            {
                FileHandle fileHandle = Platform::OpenFileHandle( ptr->filePath, ZP_OPEN_FILE_MODE_READ );

                const zp_size_t fileSize = Platform::GetFileSize( fileHandle );

                zp_uint8_t* mem = ZP_MALLOC_T_ARRAY( ptr->componentData->data.memoryLabel, zp_uint8_t, fileSize );

                //ptr->componentData->data = { .ptr = mem, .size = fileSize, .memoryLabel = ptr->componentData->data.memoryLabel };

                Platform::ReadFile( fileHandle, ptr->componentData->data.ptr, ptr->componentData->data.size );

                Platform::CloseFileHandle( fileHandle );

                ptr->componentData->hash = zp_fnv128_1a( ptr->componentData->data.ptr, ptr->componentData->data.size );
            }
        } loadFileJob {
            .componentData = data,
            .filePath = filePath,
        };

        return entity;
    }
}
