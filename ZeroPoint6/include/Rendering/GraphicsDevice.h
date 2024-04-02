//
// Created by phosg on 1/30/2022.
//

#ifndef ZP_GRAPHICSDEVICE_H
#define ZP_GRAPHICSDEVICE_H

#include <Core/Version.h>
#include "Core/Defines.h"
#include "Core/Macros.h"
#include "Core/Allocator.h"

#include "Rendering/GraphicsBuffer.h"
#include "Rendering/RenderPass.h"
#include "Rendering/GraphicsResource.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"

namespace zp
{
    struct GraphicsDeviceEnumerator
    {

    };

    struct Viewport
    {
        zp_float32_t x, y;
        zp_float32_t width, height;
        zp_float32_t minDepth, maxDepth;
    };

    struct ScissorRect
    {
        zp_int32_t x, y;
        zp_int32_t width, height;
    };

    struct StencilState
    {
        StencilOp fail;
        StencilOp pass;
        StencilOp depthFail;
        CompareOp compare;
        zp_uint32_t compareMask;
        zp_uint32_t writeMask;
        zp_uint32_t reference;
    };

    struct BlendState
    {
        zp_bool_t blendEnable;
        BlendFactor srcColorBlendFactor;
        BlendFactor dstColorBlendFactor;
        BlendOp colorBlendOp;
        BlendFactor srcAlphaBlendFactor;
        BlendFactor dstAlphaBlendFactor;
        BlendOp alphaBlendOp;
        ColorComponent writeMask;
    };

    struct VertexBinding
    {
        zp_uint32_t binding;
        zp_uint32_t stride;
        VertexInputRate inputRate;
    };

    struct VertexAttribute
    {
        zp_uint32_t location;
        zp_uint32_t binding;
        zp_uint32_t offset;
        GraphicsFormat format;
    };

    struct PipelineLayoutDesc
    {
        const char* name;
    };

    struct PipelineLayout
    {
        zp_handle_t layout;
    };

    typedef GraphicsResource<PipelineLayout> PipelineLayoutResource;

    typedef GraphicsResourceHandle<PipelineLayout> PipelineLayoutResourceHandle;

    struct GraphicsPipelineStateCreateDesc
    {
        const char* name;

        PipelineLayoutResourceHandle layout;

        RenderPassResourceHandle renderPass;
        zp_uint32_t subPass;

        // shaderHandle states
        zp_size_t shaderStageCount;
        ShaderResourceHandle shaderStages[ShaderStage_Count];

        // vertex desc
        zp_size_t vertexBindingCount;
        zp_size_t vertexAttributeCount;
        VertexBinding vertexBindings[8];
        VertexAttribute vertexAttributes[8];

        // input assembler
        Topology primitiveTopology;
        zp_bool_t primitiveRestartEnable;

        // tessellation state
        zp_uint32_t patchControlPoints;

        // viewport state
        zp_size_t viewportCount;
        Viewport viewports[16];

        zp_size_t scissorRectCount;
        ScissorRect scissorRects[16];

        // rasterizer state
        zp_bool_t depthClampEnabled;
        zp_bool_t rasterizerDiscardEnable;
        zp_bool_t depthBiasEnable;
        zp_float32_t depthBiasConstantFactor;
        zp_float32_t depthBiasClamp;
        zp_float32_t depthBiasSlopeFactor;
        zp_float32_t lineWidth;
        PolygonFillMode polygonFillMode;
        CullMode cullMode;
        FrontFaceMode frontFaceMode;

        // multisample state
        SampleCount sampleCount;
        zp_float32_t minSampleShading;
        zp_uint64_t sampleMask;
        zp_bool_t sampleShadingEnable;
        zp_bool_t alphaToCoverageEnable;
        zp_bool_t alphaToOneEnable;

        // depth stencil state
        zp_bool_t depthTestEnable;
        zp_bool_t depthWriteEnable;
        zp_bool_t depthBoundsTestEnable;
        zp_float32_t minDepthBounds;
        zp_float32_t maxDepthBounds;
        CompareOp depthCompare;

        zp_bool_t stencilTestEnable;
        StencilState front;
        StencilState back;

        // color blend state
        zp_bool_t blendLogicalOpEnable;
        LogicOp blendLogicalOp;
        zp_float32_t blendConstants[4];
        zp_size_t blendStateCount;
        BlendState blendStates[8];

        // dynamic state

    };


    struct GraphicsPipelineState
    {
        zp_hash128_t pipelineHash;

        zp_handle_t pipelineState;
    };

    struct CommandQueue
    {
        zp_uint64_t frameIndex;
        zp_handle_t commandBuffer;
        zp_handle_t commandBufferPool;
        RenderQueue queue;
    };

    //
    //
    //

    struct GraphicsDeviceDesc
    {
        String appName;
        zp_size_t stagingBufferSize;

        zp_uint32_t threadCount;
        zp_uint32_t bufferedFrameCount;

        ZP_BOOL32( geometryShaderSupport ) = true;
        ZP_BOOL32( tessellationShaderSupport ) = true;
    };

    class GraphicsDevice;

    GraphicsDevice* CreateGraphicsDevice( MemoryLabel memoryLabel, const GraphicsDeviceDesc& graphicsDeviceDesc );

    void DestroyGraphicsDevice( GraphicsDevice* graphicsDevice );

    //
    //
    //

    ZP_DECLSPEC_NOVTABLE class GraphicsDevice
    {
    ZP_NONCOPYABLE( GraphicsDevice );

    public:
        GraphicsDevice() = default;

        ~GraphicsDevice() = default;

        virtual AllocString name() const = 0;

        virtual void createSwapChain( zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height, int displayFormat, ColorSpace colorSpace ) = 0;

        virtual void destroySwapChain() = 0;

        virtual void createPerFrameData() = 0;

        virtual void destroyPerFrameData() = 0;

        virtual void beginFrame( zp_uint64_t frameIndex ) = 0;

        virtual void submit() = 0;

        virtual void present() = 0;

        virtual void waitForGPU() = 0;

#pragma region Create & Destroy Resources

        virtual void createRenderPass( const RenderPassDesc* renderPassDesc, RenderPass* renderPass ) = 0;

        virtual void destroyRenderPass( RenderPass* renderPass ) = 0;

        virtual void createGraphicsPipeline( const GraphicsPipelineStateCreateDesc* graphicsPipelineStateCreateDesc, GraphicsPipelineState* graphicsPipelineState ) = 0;

        virtual void destroyGraphicsPipeline( GraphicsPipelineState* graphicsPipelineState ) = 0;

        virtual void createPipelineLayout( const PipelineLayoutDesc* pipelineLayoutDesc, PipelineLayout* pipelineLayout ) = 0;

        virtual void destroyPipelineLayout( PipelineLayout* pipelineLayout ) = 0;

        virtual void createBuffer( const GraphicsBufferDesc& graphicsBufferDesc, GraphicsBuffer* graphicsBuffer ) = 0;

        virtual void destroyBuffer( GraphicsBuffer* graphicsBuffer ) = 0;

        virtual void createShader( const ShaderDesc& shaderDesc, Shader* shader ) = 0;

        virtual void destroyShader( Shader* shader ) = 0;

        virtual void createTexture( const TextureCreateDesc* textureCreateDesc, Texture* texture ) = 0;

        virtual void destroyTexture( Texture* texture ) = 0;

        virtual void createSampler( const SamplerCreateDesc* samplerCreateDesc, Sampler* sampler ) = 0;

        virtual void destroySampler( Sampler* sampler ) = 0;

        virtual void mapBuffer( zp_size_t offset, zp_size_t size, const GraphicsBuffer& graphicsBuffer, void** memory ) = 0;

        virtual void unmapBuffer( const GraphicsBuffer& graphicsBuffer ) = 0;

#pragma endregion

#pragma region Command Queue Operations

        virtual CommandQueue* requestCommandQueue( RenderQueue queue ) = 0;

        virtual CommandQueue* requestCommandQueue( CommandQueue* commandQueue ) = 0;

        virtual void releaseCommandQueue( CommandQueue* commandQueue ) = 0;

        virtual void beginRenderPass( const RenderPass* renderPass, CommandQueue* commandQueue ) = 0;

        virtual void nextSubpass( CommandQueue* commandQueue ) = 0;

        virtual void endRenderPass( CommandQueue* commandQueue ) = 0;

        virtual void bindPipeline( const GraphicsPipelineState* graphicsPipelineState, PipelineBindPoint bindPoint, CommandQueue* commandQueue ) = 0;

        virtual void bindIndexBuffer( const GraphicsBuffer* graphicsBuffer, IndexBufferFormat indexBufferFormat, zp_size_t offset, CommandQueue* commandQueue ) = 0;

        virtual void bindVertexBuffers( zp_uint32_t firstBinding, zp_uint32_t bindingCount, const GraphicsBuffer** graphicsBuffers, zp_size_t* offsets, CommandQueue* commandQueue ) = 0;

        virtual void updateTexture( const TextureUpdateDesc* textureUpdateDesc, const Texture* dstTexture, CommandQueue* commandQueue ) = 0;

        virtual void updateBuffer( const GraphicsBufferUpdateDesc* graphicsBufferUpdateDesc, const GraphicsBuffer* dstGraphicsBuffer, CommandQueue* commandQueue ) = 0;

#pragma endregion

#pragma region Draw Commands

        virtual void draw( zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance, CommandQueue* commandQueue ) = 0;

        virtual void drawIndirect( const GraphicsBuffer& buffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue ) = 0;

        virtual void drawIndexed( zp_uint32_t indexCount, zp_uint32_t instanceCount, zp_uint32_t firstIndex, zp_int32_t vertexOffset, zp_uint32_t firstInstance, CommandQueue* commandQueue ) = 0;

        virtual void drawIndexedIndirect( const GraphicsBuffer& buffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue ) = 0;

#pragma endregion

#pragma region Compute Dispatch Commands

        virtual void dispatch( zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ, CommandQueue* commandQueue ) = 0;

        virtual void dispatchIndirect( const GraphicsBuffer& buffer, CommandQueue* commandQueue ) = 0;

#pragma endregion

#pragma region Profiler Markers

        virtual void beginEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue ) = 0;

        virtual void endEventLabel( CommandQueue* commandQueue ) = 0;

        virtual void markEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue ) = 0;

#pragma endregion
    };
}

#endif //ZP_GRAPHICSDEVICE_H
