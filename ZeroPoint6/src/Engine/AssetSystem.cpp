//
//
//

#include "Platform/Platform.h"

#include "Engine/AssetSystem.h"
#include "Engine/MemoryLabels.h"

#include "Engine/TransformComponent.h"

namespace zp
{
    AssetSystem::AssetSystem( MemoryLabel memoryLabel )
        : m_pendingAssetLoadCommands( 4, memoryLabel )
        , m_currentlyLoadingAssets( 4, memoryLabel )
        , m_pendingAssetLoadCommandQueue( 4, memoryLabel )
        , m_assetLoadCommandPool( 4, memoryLabel )
        , m_loadedAssetIndexMap( 16, memoryLabel )
        , m_loadedAssets( 16, memoryLabel )
        , m_loadedAssetFreeListIndices( 4, memoryLabel )
        , m_combinedAssetManifestEntries( 16, memoryLabel )
        , m_virtualFileSystem( nullptr )
        , m_maxActiveAssetLoadCommands( 4 )
        , m_jobSystem( nullptr )
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
        // destroy loaded assets
        for( AssetData& assetData : m_loadedAssets )
        {
            if( assetData.data != nullptr )
            {
                GetAllocator( assetData.memoryLabel )->free( assetData.data );
                assetData.data = nullptr;
            }
        }
        m_loadedAssets.destroy();
        m_loadedAssetIndexMap.destroy();
        m_loadedAssetFreeListIndices.destroy();

        // move currently loading assets to pool for deletion
        for( AssetLoadCommand* alc : m_currentlyLoadingAssets )
        {
            m_assetLoadCommandPool.pushBack( alc );
        }
        m_currentlyLoadingAssets.destroy();

        // move pending loads to pool for deletion
        while( !m_pendingAssetLoadCommandQueue.isEmpty() )
        {
            m_assetLoadCommandPool.pushBack( m_pendingAssetLoadCommandQueue.dequeue() );
        }
        m_pendingAssetLoadCommandQueue.destroy();

        // delete load commands
        for( AssetLoadCommand* alc : m_assetLoadCommandPool )
        {
            ZP_FREE_( memoryLabel, alc );
        }
        m_assetLoadCommandPool.destroy();
    }

    void AssetSystem::setup( JobSystem* jobSystem, EntityComponentManager* entityComponentManager )
    {
        m_jobSystem = jobSystem;
        m_entityComponentManager = entityComponentManager;
    }

    void AssetSystem::teardown()
    {
        m_jobSystem = nullptr;
        m_entityComponentManager = nullptr;
    }

    void AssetSystem::setAssetTypeMemoryLabel( AssetTypes assetType, MemoryLabel assetMemoryLabel )
    {
        m_assetMemoryLabels[ static_cast<AssetType>( assetType ) ] = assetMemoryLabel;
    }

    void AssetSystem::loadAssetManifest( const zp_hash128_t& assetManifestVersion )
    {
    }

    zp_bool_t AssetSystem::isReady() const
    {
        return true;
    }

    void AssetSystem::loadAssetAsync( const zp_guid128_t& assetGUID, AssetLoadCompleteCallback assetLoadCompleteCallback, void* userPtr )
    {
        AssetLoadCommand* assetLoadCommand;
        zp_size_t index;

        // if the asset is already loaded, perform callback
        if( m_loadedAssetIndexMap.get( assetGUID, &index ) )
        {
            AssetData& assetData = m_loadedAssets[ index ];
            assetData.refCount++;

            if( assetLoadCompleteCallback )
            {
                assetLoadCompleteCallback( assetData.data, assetData.size, userPtr );
            }
        }
            // if the asset is pending, add to the existing chain
        else if( m_pendingAssetLoadCommands.get( assetGUID, &assetLoadCommand ) )
        {
            while( assetLoadCommand->nextCommand != nullptr )
            {
                assetLoadCommand = assetLoadCommand->nextCommand;
            }

            AssetLoadCommand* requestedAssetLoadCommand = requestAssetLoadCommand();
            requestedAssetLoadCommand->assetGUID = assetGUID;
            requestedAssetLoadCommand->callback = assetLoadCompleteCallback;
            requestedAssetLoadCommand->userPtr = userPtr;
            requestedAssetLoadCommand->nextCommand = nullptr;

            assetLoadCommand->nextCommand = requestedAssetLoadCommand;
        }
            // otherwise, new load command
        else
        {
            AssetLoadCommand* requestedAssetLoadCommand = requestAssetLoadCommand();
            *requestedAssetLoadCommand = {};
            requestedAssetLoadCommand->assetGUID = assetGUID;
            requestedAssetLoadCommand->callback = assetLoadCompleteCallback;
            requestedAssetLoadCommand->userPtr = userPtr;
            requestedAssetLoadCommand->nextCommand = nullptr;

            m_pendingAssetLoadCommands.set( assetGUID, requestedAssetLoadCommand );

            m_pendingAssetLoadCommandQueue.enqueue( requestedAssetLoadCommand );
        }
    }

    void AssetSystem::unloadAsset( const zp_guid128_t& assetGUID )
    {
        zp_size_t index;
        if( m_loadedAssetIndexMap.get( assetGUID, &index ) )
        {
            AssetData& assetData = m_loadedAssets[ index ];
            if( assetData.refCount > 0 )
            {
                assetData.refCount--;
            }
        }
    }

    void AssetSystem::unloadAllAssets()
    {
        for( AssetData& assetData : m_loadedAssets )
        {
            assetData.refCount = 0;
        }
    }

    void AssetSystem::garbageCollectAssets()
    {
        Vector<zp_size_t> indicesToDelete( getLoadedAssetCount(), MemoryLabels::Temp );

        for( zp_size_t i = 0; i < m_loadedAssets.size(); ++i )
        {
            const AssetData& assetData = m_loadedAssets[ i ];
            if( assetData.data && assetData.refCount == 0 )
            {
                indicesToDelete.pushBack( i );
            }
        }

        for( const zp_size_t index : indicesToDelete )
        {
            AssetData& assetData = m_loadedAssets[ index ];

            GetAllocator( assetData.memoryLabel )->free( assetData.data );

            assetData = {};

            m_loadedAssetFreeListIndices.pushBack( index );
        }
    }

    zp_size_t AssetSystem::getLoadedAssetCount() const
    {
        const zp_size_t loadedAssetCount = m_loadedAssets.size() - m_loadedAssetFreeListIndices.size();
        return loadedAssetCount;
    }

    PreparedJobHandle AssetSystem::process( JobSystem* jobSystem, EntityComponentManager* entityComponentManager, const PreparedJobHandle& inputHandle )
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
            .data = {},
            .memoryLabel = MemoryLabels::FileIO,
        };

        struct LoadFileJob
        {
            RawAssetComponentData* componentData;
            const char* filePath;

            ZP_JOB_DEBUG_NAME( LoadFileJob );

            static void Execute( const JobHandle& parentJobHandle, const LoadFileJob* ptr )
            {
                zp_handle_t fileHandle = GetPlatform()->OpenFileHandle( ptr->filePath, ZP_OPEN_FILE_MODE_READ );

                const zp_size_t fileSize = GetPlatform()->GetFileSize( fileHandle );

                zp_uint8_t* mem = ZP_MALLOC_T_ARRAY( ptr->componentData->memoryLabel, zp_uint8_t, fileSize );

                ptr->componentData->data = { .ptr = mem, .length = fileSize };

                GetPlatform()->ReadFile( fileHandle, ptr->componentData->data.ptr, ptr->componentData->data.length );

                GetPlatform()->CloseFileHandle( fileHandle );

                ptr->componentData->hash = zp_fnv128_1a( ptr->componentData->data.ptr, ptr->componentData->data.length );
            }
        } loadFileJob {
            .componentData = data,
            .filePath = filePath,
        };

        PreparedJobHandle jobHandle = m_jobSystem->PrepareJobData( loadFileJob );

        m_jobSystem->Schedule( jobHandle ).complete();

        return entity;
    }

    AssetSystem::AssetLoadCommand* AssetSystem::requestAssetLoadCommand()
    {
        AssetLoadCommand* cmd = nullptr;

        if( m_assetLoadCommandPool.isEmpty() )
        {
            cmd = ZP_MALLOC_T( memoryLabel, AssetLoadCommand );
        }
        else
        {
            cmd = m_assetLoadCommandPool.back();
            m_assetLoadCommandPool.popBack();
        }

        return cmd;
    }

    void AssetSystem::releaseAssetLoadCommand( AssetLoadCommand* assetLoadCommand )
    {
        *assetLoadCommand = {};

        m_assetLoadCommandPool.pushBack( assetLoadCommand );
    }

    void AssetSystem::startAssetLoadCommand( JobSystem* jobSystem, AssetLoadCommand* assetLoadCommand )
    {
        m_currentlyLoadingAssets.pushBack( assetLoadCommand );

        AssetManifestEntry* assetManifestEntryPtr;
        if( m_combinedAssetManifestEntries.get( assetLoadCommand->assetGUID, &assetManifestEntryPtr ) )
        {
            assetLoadCommand->assetType = assetManifestEntryPtr->assetType;
            assetLoadCommand->size = assetManifestEntryPtr->assetPackUncompressedSize;
            assetLoadCommand->memoryLabel = m_assetMemoryLabels[ assetLoadCommand->assetType ];

            zp_size_t indexPtr;
            m_loadedAssetIndexMap.get( assetManifestEntryPtr->assetPack, &indexPtr );
            const AssetData& assetPackData = m_loadedAssets[ indexPtr ];

            if( assetManifestEntryPtr->assetPackCompressedSize != assetManifestEntryPtr->assetPackUncompressedSize )
            {
                assetLoadCommand->buffer = static_cast<zp_uint8_t*>(GetAllocator( assetLoadCommand->memoryLabel )->allocate( assetLoadCommand->size, kDefaultMemoryAlignment ));

                zp_lzf_expand( assetPackData.data, assetManifestEntryPtr->assetPackOffset, assetManifestEntryPtr->assetPackCompressedSize, assetLoadCommand->buffer, 0, assetLoadCommand->size );
            }
            else
            {
                assetLoadCommand->buffer = assetPackData.data + assetManifestEntryPtr->assetPackOffset;
            }

            assetLoadCommand->errorCode = 0;
        }
        else
        {
            assetLoadCommand->errorCode = -1;
        }

        finalizeAssetLoadCommand( assetLoadCommand );
    }

    void AssetSystem::finalizeAssetLoadCommand( AssetLoadCommand* assetLoadCommand )
    {
        m_pendingAssetLoadCommands.remove( assetLoadCommand->assetGUID );
        m_currentlyLoadingAssets.eraseSwapBack( assetLoadCommand );

        AssetLoadCommand* currentAssetLoadCommand = assetLoadCommand;

        const zp_bool_t success = currentAssetLoadCommand->errorCode == 0;
        if( success )
        {
            AssetData assetData {
                assetLoadCommand->buffer,
                assetLoadCommand->size,
                1,
                assetLoadCommand->assetType,
                assetLoadCommand->memoryLabel
            };

            zp_size_t index;
            if( !m_loadedAssetFreeListIndices.isEmpty() )
            {
                index = m_loadedAssetFreeListIndices.back();
                m_loadedAssetFreeListIndices.popBack();
            }
            else
            {
                index = m_loadedAssets.size();
                m_loadedAssets.pushBackEmpty();
            }

            m_loadedAssets[ index ] = assetData;
            m_loadedAssetIndexMap.set( assetLoadCommand->assetGUID, index );
        }

        const zp_uint8_t* buffer = success ? currentAssetLoadCommand->buffer : nullptr;
        const zp_size_t size = success ? currentAssetLoadCommand->size : 0;

        while( currentAssetLoadCommand != nullptr )
        {
            if( currentAssetLoadCommand->callback )
            {
                currentAssetLoadCommand->callback( buffer, size, currentAssetLoadCommand->userPtr );
            }

            AssetLoadCommand* next = currentAssetLoadCommand->nextCommand;
            releaseAssetLoadCommand( currentAssetLoadCommand );
            currentAssetLoadCommand = next;
        }
    }
}
