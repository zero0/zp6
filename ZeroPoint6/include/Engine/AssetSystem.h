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
#include "Core/Job.h"

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
        Text,

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

    struct AssetSource
    {

    };

    class VirtualFileSystem
    {
    public:
        [[nodiscard]] AssetSource getAssetSource() const;

        void addAssetSource();
    };

    //
    //
    //

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
        AllocMemory data;
    };

    struct MeshAssetViewComponentData
    {
    };

    struct ShaderAssetViewComponentData
    {
    };

    struct ComputeShaderAssetViewComponentData
    {
    };

    struct AssetReferenceCountComponentData
    {
        zp_int32_t refCount;
    };

    //
    //
    //

    class AssetSystem
    {
    public:
        explicit AssetSystem( MemoryLabel memoryLabel );

        ~AssetSystem();

        void setup( EntityComponentManager* entityComponentManager );

        void teardown();

        //

        [[nodiscard]] JobHandle process( EntityComponentManager* entityComponentManager, const JobHandle& inputHandle );

        Entity loadFileDirect( const char* filePath );

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

        void startAssetLoadCommand( AssetLoadCommand* assetLoadCommand );

        void finalizeAssetLoadCommand( AssetLoadCommand* assetLoadCommand );

        //

        JobSystem* m_jobSystem;
        EntityComponentManager* m_entityComponentManager;
        VirtualFileSystem* m_virtualFileSystem;

        MemoryLabel m_assetMemoryLabels[static_cast<zp_size_t>(AssetTypes::AssetTypes_Count )];

    public:
        const MemoryLabel memoryLabel;
    };
}

#endif //ZP_ASSETSYSTEM_H
