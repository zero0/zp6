//
// Created by phosg on 3/7/2025.
//

#include "Core/Types.h"
#include "Core/Common.h"
#include "Core/Allocator.h"
#include "Core/Memory.h"
#include "Core/Vector.h"
#include "Core/Data.h"
#include "Core/Math.h"
#include "Core/String.h"
#include "Core/Log.h"

#include "Rendering/Texture.h"
#include "Rendering/GraphicsDefines.h"

#define STBI_NO_STDIO
#include "stb/stb_image.h"

namespace zp
{
    namespace
    {
        // KTX2 Format
        const FixedArray kFileIdentifier {
            //'«', 'K', 'T', 'X', ' ', '2', '0', '»', '\r', '\n', '\x1A', '\n'
            static_cast<zp_uint8_t>(0xAB), 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
        };

        struct KTX2Header
        {
            FixedArray<zp_uint8_t, kFileIdentifier.length()> identifier;
            zp_uint32_t vkFormat;
            zp_uint32_t typeSize;
            zp_uint32_t pixelWidth;
            zp_uint32_t pixelHeight;
            zp_uint32_t pixelDepth;
            zp_uint32_t layerCount;
            zp_uint32_t faceCount;
            zp_uint32_t levelCount;
            zp_uint32_t supercompressionScheme;
        };

        struct KTX2Index
        {
            zp_uint32_t dfdByteOffset;
            zp_uint32_t dfdByteLength;
            zp_uint32_t kvdByteOffset;
            zp_uint32_t kvdByteLength;
            zp_uint64_t sgdByteOffset;
            zp_uint64_t sgdByteLength;
        };

        struct KTX2Level
        {
            zp_uint64_t byteOffset;
            zp_uint64_t byteLength;
            zp_uint64_t uncompressedByteLength;
        };

        struct KTX2KeyValue
        {
            String key;
            String value;
        };

        zp_uint32_t MaxMipLevels( const Size3Du& size )
        {
            const zp_uint32_t maxSize = zp_max( size.width, size.height );
            ZP_ASSERT( zp_is_pow2( maxSize ) );

            return Math::Log2( maxSize );
        }

        Size3Du GetMipSize( const Size3Du& size, zp_uint32_t mip )
        {
            const Size3Du mipSize {
                .width = zp_max( 1U, size.width ),
                .height = zp_max( 1U, size.height ),
                .depth = size.depth,
            };
            return mipSize;
        }

        zp_size_t GetTextureMemorySize( const Size3Du& size, zp_uint32_t pixelsPerBlock, zp_uint32_t blockSize )
        {
            // pixelsPerBLock = 1 (uncompressed)
            //                  16 (block compressed 4x4)
            const zp_uint32_t memorySize = ( size.width * size.height / pixelsPerBlock ) * size.depth * blockSize;
            return memorySize;
        }

        zp_uint8_t* GetColorAt( RawTextureData& textureData, zp_uint32_t x, zp_uint32_t y, zp_uint32_t z )
        {
            const zp_ptrdiff_t colorSize = textureData.componentCount * textureData.componentSize;
            const zp_ptrdiff_t xOffset = static_cast<zp_ptrdiff_t>(x) * colorSize;
            const zp_ptrdiff_t yOffset = static_cast<zp_ptrdiff_t>(y) * textureData.size.width * colorSize;
            const zp_ptrdiff_t zOffset = static_cast<zp_ptrdiff_t>(z) * textureData.size.width * textureData.size.height * colorSize;
            return textureData.data.data() + xOffset + yOffset + zOffset;
        }

        const zp_uint8_t* GetColorAt( const RawTextureData& textureData, zp_uint32_t x, zp_uint32_t y, zp_uint32_t z )
        {
            const zp_ptrdiff_t colorSize = textureData.componentCount * textureData.componentSize;
            const zp_ptrdiff_t xOffset = static_cast<zp_ptrdiff_t>(x) * colorSize;
            const zp_ptrdiff_t yOffset = static_cast<zp_ptrdiff_t>(y) * textureData.size.width * colorSize;
            const zp_ptrdiff_t zOffset = static_cast<zp_ptrdiff_t>(z) * textureData.size.width * textureData.size.height * colorSize;
            return textureData.data.data() + xOffset + yOffset + zOffset;
        }

        Color ReadColorAt( const RawTextureData& textureData, zp_uint32_t x, zp_uint32_t y, zp_uint32_t z )
        {
            Color color {};

            const zp_float32_t invScale = 1.0F / 255.0F;

            const zp_uint8_t* rawColor = GetColorAt( textureData, x, y, z );

            for( zp_uint32_t component = 0; component < textureData.componentCount; ++component )
            {
                switch( textureData.componentSize )
                {
                    case sizeof( zp_uint8_t ):
                        color.rgba[ component ] = rawColor[ component ] * invScale;
                        break;
                    case sizeof( zp_float16_t ):
                        // TODO: no f16 to f32?
                        color.rgba[ component ] = reinterpret_cast<const zp_float16_t*>(rawColor)[ component ];
                        break;
                    case sizeof( zp_float32_t ):
                        color.rgba[ component ] = reinterpret_cast<const zp_float32_t*>(rawColor)[ component ];
                        break;
                }
            }

            return color;
        }

        void WriteColorAt( const Color& color, RawTextureData& textureData, zp_uint32_t x, zp_uint32_t y, zp_uint32_t z )
        {
            const zp_float32_t scale = 255.0F;

            zp_uint8_t* rawColor = GetColorAt( textureData, x, y, z );

            for( zp_uint32_t component = 0; component < textureData.componentCount; ++component )
            {
                switch( textureData.componentSize )
                {
                    case sizeof( zp_uint8_t ):
                        rawColor[ component ] = zp_floor_to_int( color.rgba[ component ] * scale ) & 0xFF;
                        break;
                    case sizeof( zp_float16_t ):
                        // TODO: no f32 to f16?
                        reinterpret_cast<zp_float16_t*>(rawColor)[ component ] = color.rgba[ component ];
                        break;
                    case sizeof( zp_float32_t ):
                        reinterpret_cast<zp_float32_t*>(rawColor)[ component ] = color.rgba[ component ];
                        break;
                }
            }
        }

        void CalcComponentSwizzle( Color& dstColor, const Color& srcColor, const zp_uint32_t component, const ComponentSwizzle swizzle )
        {
            switch( swizzle )
            {
                case Identity:
                    dstColor.rgba[ component ] = srcColor.rgba[ component ];
                    break;
                case R:
                    dstColor.rgba[ component ] = srcColor.r;
                    break;
                case G:
                    dstColor.rgba[ component ] = srcColor.g;
                    break;
                case B:
                    dstColor.rgba[ component ] = srcColor.b;
                    break;
                case A:
                    dstColor.rgba[ component ] = srcColor.a;
                    break;
                case Zero:
                    dstColor.rgba[ component ] = 0;
                    break;
                case One:
                    dstColor.rgba[ component ] = 1;
                    break;
            }
        }

        Color ColorSwizzle( const Color& srcColor, const ColorSwizzle& colorSwizzle )
        {
            Color color {};

            CalcComponentSwizzle( color, srcColor, 0, colorSwizzle.r );
            CalcComponentSwizzle( color, srcColor, 1, colorSwizzle.g );
            CalcComponentSwizzle( color, srcColor, 2, colorSwizzle.b );
            CalcComponentSwizzle( color, srcColor, 3, colorSwizzle.a );

            return color;
        }

        void GenerateMipData( const TextureExportConfig& config, zp_uint32_t mip, const RawTextureData& srcPrevMipData, RawTextureData& dstMipData )
        {
            for( zp_uint32_t z = 0; z < srcPrevMipData.size.depth; ++z )
            {
                for( zp_uint32_t y = 0, my = 0; y < srcPrevMipData.size.height; y += 2, ++my )
                {
                    for( zp_uint32_t x = 0, mx = 0; x < srcPrevMipData.size.width; x += 2, ++mx )
                    {
                        Color mipColor {};

                        switch( config.mipDownSampleFunction )
                        {
                            case Point:
                            {
                                mipColor = ReadColorAt( srcPrevMipData, x, y, z );
                            }
                            break;

                            case Bilinear:
                            {
                                const Color c00 = ReadColorAt( srcPrevMipData, x + 0, y + 0, z );
                                const Color c01 = ReadColorAt( srcPrevMipData, x + 0, y + 1, z );
                                const Color c10 = ReadColorAt( srcPrevMipData, x + 1, y + 0, z );
                                const Color c11 = ReadColorAt( srcPrevMipData, x + 1, y + 1, z );

                                mipColor = Math::Add( Math::Add( c00, c11 ), Math::Add( c10, c01 ) );
                                mipColor = Math::Mul( mipColor, 0.25F );
                            }
                            break;

                            default:
                                ZP_INVALID_CODE_PATH();
                                break;
                        }

                        // TODO: fade mip

                        WriteColorAt( mipColor, dstMipData, mx, my, z );
                    }
                }
            }
        }

        Memory CompressTexture( const TextureExportConfig& config, const RawTextureData& srcData, ArenaAllocator& arenaAllocator )
        {
            Memory compressedTexture {};

            // TODO: have fast path for each format? or always convert because fast path is probably not the default case
            // TODO: perform swizzle just before storing / compressing? or should that be done during initial import?

            switch( config.format )
            {
                case ZP_GRAPHICS_FORMAT_R8G8B8A8_UNORM:
                {
                    if( srcData.componentCount == 4 && srcData.componentSize == sizeof( zp_uint8_t ) )
                    {
                        compressedTexture = { .ptr = (void*)srcData.data.data(), .size = srcData.data.size() };
                    }
                    else
                    {

                    }
                }
                break;

                case ZP_GRAPHICS_FORMAT_BC1_RGB_UNORM:
                case ZP_GRAPHICS_FORMAT_BC1_RGB_SRGB:
                case ZP_GRAPHICS_FORMAT_BC1_RGBA_UNORM:
                case ZP_GRAPHICS_FORMAT_BC1_RGBA_SRGB:
                {
                    // TODO: BC1 compression
                }
                break;

                default:
                    Log::info() << "Unknown texture format: " << config.format << Log::endl;
                    break;
            }

            return compressedTexture;
        }

        void LoadRawTextureDataFromMemory( const ReadOnlyMemory textureData, RawTextureData& rawTextureData )
        {
            zp_int32_t width = 0;
            zp_int32_t height = 0;
            zp_int32_t channels = 0;

            zp_uint8_t* rawImageData = stbi_load_from_memory( static_cast<const zp_uint8_t*>(textureData.ptr), static_cast<zp_int32_t>(textureData.size), &width, &height, &channels, 0 );
            ZP_ASSERT_MSG_ARGS( rawImageData != nullptr, "%s", stbi_failure_reason() );

            const zp_size_t componentSize = sizeof( zp_uint8_t );
            const zp_size_t rawImageSize = componentSize * width * height * channels;

            rawTextureData = {
                .size {
                    .width = zp_uint32_t(width),
                    .height = zp_uint32_t(height),
                    .depth = 1U,
                },
                .componentCount = zp_uint32_t(channels),
                .componentSize = componentSize,
                .data = MemoryArray( rawImageData, rawImageSize ),
            };
        }

        void DestroyRawTextureData( RawTextureData& rawTextureData )
        {
            stbi_image_free( rawTextureData.data.data() );

            rawTextureData = {};
        }
    }

    //
    //
    //

    void AllocateTextureData( MemoryLabel memoryLabel, Memory externalTextureData, TextureData& data )
    {
        data = {};

        DataStreamReader reader( externalTextureData );

        KTX2Header header;
        reader.read( header );

        ZP_ASSERT( zp_memcmp( header.identifier.data(), header.identifier.length(), kFileIdentifier.data(), kFileIdentifier.length() ) == 0 );


    }

    void FreeTextureData( TextureData& data )
    {
        ZP_SAFE_FREE( data.memoryLabel, data.data.ptr );
        data = {};
    }

    //
    //
    //

    void ExportRawTextureData( const TextureExportConfig& config, const RawTextureData& srcData, DataStreamWriter& dstDataStream )
    {
        const Size3Du minSize {
            .width = 1,
            .height = 1,
            .depth = 0,
        };
        const Size3Du srcSize {
            .width = zp_clamp( srcData.size.width, minSize.width, config.maxSize.width ),
            .height = zp_clamp( srcData.size.height, minSize.height, config.maxSize.height ),
            .depth = zp_clamp( srcData.size.depth, minSize.depth, config.maxSize.depth ),
        };

        const zp_uint32_t maxMipLevels = MaxMipLevels( srcSize );
        ZP_ASSERT( config.baseMipLevel < maxMipLevels );

        const zp_uint32_t baseMipLevel = config.baseMipLevel;
        const zp_uint32_t mipLevelCount = config.generateMipMaps ? maxMipLevels - baseMipLevel : 1;
        const zp_uint32_t layerCount = 1;
        const zp_uint32_t faceCount = 1;
        const zp_uint32_t supercompressionScheme = 0;

        const Size3Du baseMipSize = GetMipSize( srcSize, baseMipLevel );

        ArenaAllocator arenaAllocator( FixedArenaMemoryStorage( MemoryLabels::Temp, 16 MB ) );

        Vector<Memory> compressedMipLevels( mipLevelCount, MemoryLabels::Temp );

        // generate all mip levels
        if( config.generateMipMaps )
        {
            Vector<RawTextureData> allMipLevels( maxMipLevels, MemoryLabels::Temp );

            // add mip 0
            allMipLevels.pushBack( srcData );

            // generate each mip using previous mip
            for( zp_uint32_t mip = 1; mip < maxMipLevels; ++mip )
            {
                const Size3Du mipSize = GetMipSize( srcSize, mip );

                const zp_size_t mipDataSize = GetTextureMemorySize( mipSize, 1, srcData.componentCount * srcData.componentSize );

                RawTextureData mipLevel {
                    .size = mipSize,
                    .componentCount = srcData.componentCount,
                    .componentSize = srcData.componentCount,
                    .data { (zp_uint8_t*)arenaAllocator.allocate( mipDataSize, kDefaultMemoryAlignment ), mipDataSize },
                };

                GenerateMipData( config, mip, allMipLevels[ mip - 1 ], mipLevel );

                allMipLevels.pushBack( mipLevel );
            }

            // compress only mips needed
            for( zp_uint32_t mip = baseMipLevel, maxMip = baseMipLevel + mipLevelCount; mip < maxMip; ++mip )
            {
                Memory compressedMipMemory = CompressTexture( config, allMipLevels[ mip ], arenaAllocator );

                compressedMipLevels.pushBack( compressedMipMemory );
            }
        }
        // just compress mip 0
        else
        {
            Memory compressedMipMemory = CompressTexture( config, srcData, arenaAllocator );

            compressedMipLevels.pushBack( compressedMipMemory );
        }

        // header
        const KTX2Header header {
            .identifier = kFileIdentifier,
            .vkFormat = static_cast<zp_uint32_t>(config.format),
            .typeSize = 1,
            .pixelWidth = baseMipSize.width,
            .pixelHeight = baseMipSize.height,
            .pixelDepth = baseMipSize.depth,
            .layerCount = layerCount,
            .faceCount = faceCount,
            .levelCount = mipLevelCount,
            .supercompressionScheme = 0,
        };
        dstDataStream.write( header );

        // index
        KTX2Index index {};
        const zp_ptrdiff_t indexOffset = dstDataStream.write( index );

        // mip levels
        FixedArray<zp_ptrdiff_t, 16> mipLevelOffsets;
        for( zp_uint32_t i = 0; i < mipLevelCount; ++i )
        {
            const KTX2Level level {};
            mipLevelOffsets[ i ] = dstDataStream.write( level );
        }

        // dfd
        {
            index.dfdByteOffset = dstDataStream.write<zp_uint32_t>( 0 );

            // TODO: write actual dfd
            int dfdData;
            dstDataStream.write( dfdData );

            index.dfdByteLength = dstDataStream.position() - index.dfdByteOffset;
            dstDataStream.writeAt( index.dfdByteLength, index.dfdByteOffset, DataStreamSeekOrigin::Exact );
        }

        // key/value
        {
            index.kvdByteOffset = dstDataStream.write<zp_uint32_t>( 0 );

            // TODO: write actual key/value
            FixedVector<KTX2KeyValue, 16> kvdKeyValues;
            kvdKeyValues.pushBack( {
                .key = String::As( "KTXwriter" ),
                .value = String::As( "" ),
            } );
            kvdKeyValues.pushBack( {
                .key = String::As( "KTXwriterScParams" ),
                .value = String::As( "" ),
            } );

            for( const KTX2KeyValue& kv : kvdKeyValues )
            {
                const zp_ptrdiff_t kvOffset = dstDataStream.write<zp_uint32_t>( 0 );
                dstDataStream.write( kv.key.c_str(), kv.key.length() );
                dstDataStream.write( '\0' );
                dstDataStream.write( kv.value.c_str(), kv.value.length() );

                const zp_uint32_t kvLength = dstDataStream.position() - kvOffset;

                // alignment not included in kvLength
                dstDataStream.writeAlignment( 4 );

                dstDataStream.writeAt( kvLength, kvOffset, DataStreamSeekOrigin::Exact );
            }

            index.kvdByteLength = dstDataStream.position() - index.kvdByteOffset;
        }

        // sgd
        {
            if( header.supercompressionScheme != 0 )
            {
                index.sgdByteOffset = dstDataStream.write<zp_uint32_t>( 0 );

                index.sgdByteLength = dstDataStream.position() - index.sgdByteOffset;
            }
        }

        // level
        {
            for( zp_uint32_t mip = 0; mip < mipLevelCount; ++mip )
            {
                const Memory& compressedMipLevel = compressedMipLevels[ mip ];

                const KTX2Level level {
                    .byteOffset = dstDataStream.position(),
                    .byteLength = compressedMipLevel.size,
                    .uncompressedByteLength = supercompressionScheme == 0 ? compressedMipLevel.size : 0,
                };
                dstDataStream.write( compressedMipLevel.ptr, compressedMipLevel.size );

                // write back level index
                dstDataStream.writeAt( level, mipLevelOffsets[ mip ], DataStreamSeekOrigin::Exact );
            }
        }

        // write back index
        dstDataStream.writeAt( index, indexOffset, DataStreamSeekOrigin::Exact );
    }

}
