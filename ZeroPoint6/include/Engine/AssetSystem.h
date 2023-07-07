//
// Created by phosg on 2/23/2022.
//

#ifndef ZP_ASSETSYSTEM_H
#define ZP_ASSETSYSTEM_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Map.h"
#include "Core/Allocator.h"
#include "Core/Vector.h"
#include "Core/Queue.h"

#include "Engine/JobSystem.h"
#include "Engine/Entity.h"
#include "Engine/EntityComponentManager.h"

namespace zp
{
    typedef void (* AssetLoadCompleteCallback)( const zp_uint8_t* data, const zp_size_t size, void* userPtr );

    typedef zp_uint32_t AssetType;

    enum class AssetTypes : AssetType
    {
        Raw,
        Data,

        AssetManifest,
        AssetPack,

        Prefab,

        Mesh,
        Animation,
        Texture,
        Shader,
        Material,

        Audio,

        AssetTypes_Count,
    };

    class VirtualFileSystem
    {
    public:
    };

    template<AssetType AT>
    struct AssetReferenceComponentData
    {
        constexpr AssetType assetType()
        {
            return AT;
        }

        Entity assetEntity;
    };

    struct RawAssetComponentData
    {
        zp_guid128_t guid;
        zp_hash128_t hash;
        MemoryArray<zp_uint8_t> data;
        const MemoryLabel memoryLabel;
    };

    struct MeshAssetViewComponentData
    {
    };

    struct AssetReferenceCountComponentData
    {
        zp_int32_t refCount;
    };

    class AssetSystem
    {
    public:
        explicit AssetSystem( MemoryLabel memoryLabel );

        ~AssetSystem();

        void setup();

        void teardown();

        void setAssetTypeMemoryLabel( AssetTypes assetType, MemoryLabel assetMemoryLabel );

        void loadAssetManifest( const zp_hash128_t& assetManifestVersion );

        [[nodiscard]] zp_bool_t isReady() const;

        void loadAssetAsync( const zp_guid128_t& assetGUID, AssetLoadCompleteCallback assetLoadCompleteCallback, void* userPtr = nullptr );

        void unloadAsset( const zp_guid128_t& assetGUID );

        void unloadAllAssets();

        void garbageCollectAssets();

        [[nodiscard]] zp_size_t getLoadedAssetCount() const;

        [[nodiscard]] PreparedJobHandle process( JobSystem* jobSystem, EntityComponentManager* entityComponentManager, const PreparedJobHandle& inputHandle );

    private:
        struct AssetData
        {
            zp_uint8_t* data;
            zp_size_t size;
            zp_size_t refCount;
            AssetType assetType;
            MemoryLabel memoryLabel;
        };

        struct AssetLoadCommand
        {
            zp_guid128_t assetGUID;
            AssetLoadCompleteCallback callback;
            void* userPtr;
            zp_uint8_t* buffer;
            zp_size_t size;
            AssetLoadCommand* nextCommand;
            AssetType assetType;
            zp_int32_t errorCode;
            MemoryLabel memoryLabel;
        };

        struct AssetManifestEntry
        {
            zp_guid128_t assetPack;
            zp_size_t assetPackOffset;
            zp_size_t assetPackCompressedSize;
            zp_size_t assetPackUncompressedSize;
            AssetType assetType;
        };

        AssetLoadCommand* requestAssetLoadCommand();

        void releaseAssetLoadCommand( AssetLoadCommand* assetLoadCommand );

        void startAssetLoadCommand( JobSystem* jobSystem, AssetLoadCommand* assetLoadCommand );

        void finalizeAssetLoadCommand( AssetLoadCommand* assetLoadCommand );

        Map<zp_guid128_t, AssetLoadCommand*, zp_hash128_t, CastEqualityComparer<zp_guid128_t, zp_hash128_t>> m_pendingAssetLoadCommands;
        Vector<AssetLoadCommand*> m_currentlyLoadingAssets;
        Queue<AssetLoadCommand*> m_pendingAssetLoadCommandQueue;

        Vector<AssetLoadCommand*> m_assetLoadCommandPool;

        Map<zp_guid128_t, zp_size_t, zp_hash128_t, CastEqualityComparer<zp_guid128_t, zp_hash128_t>> m_loadedAssetIndexMap;
        Vector<AssetData> m_loadedAssets;
        Vector<zp_size_t> m_loadedAssetFreeListIndices;

        Map<zp_guid128_t, AssetManifestEntry, zp_hash128_t, CastEqualityComparer<zp_guid128_t, zp_hash128_t>> m_combinedAssetManifestEntries;

        VirtualFileSystem* m_virtualFileSystem;

        zp_size_t m_maxActiveAssetLoadCommands;

        MemoryLabel m_assetMemoryLabels[static_cast<zp_size_t>(AssetTypes::AssetTypes_Count )];

    public:
        MemoryLabel memoryLabel;
    };
}

#endif //ZP_ASSETSYSTEM_H
