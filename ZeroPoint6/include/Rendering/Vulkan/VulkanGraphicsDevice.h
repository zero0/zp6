//
// Created by phosg on 2/3/2022.
//

#ifndef ZP_VULKANGRAPHICSDEVICE_H
#define ZP_VULKANGRAPHICSDEVICE_H

#include "Core/Defines.h"
#include "Core/Types.h"
#include "Core/Macros.h"
#include "Core/Vector.h"

#include "Rendering/GraphicsDevice.h"

#include <vulkan/vulkan.h>

namespace zp
{
    enum
    {
        kBufferedFrameCount = 4,
    };


    class VulkanGraphicsDevice final : public GraphicsDevice
    {
    ZP_NONCOPYABLE( VulkanGraphicsDevice );

    public:
        VulkanGraphicsDevice( MemoryLabel memoryLabel, GraphicsDeviceFeatures graphicsDeviceFeatures );

        ~VulkanGraphicsDevice() final;

        void createSwapChain( zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height, int displayFormat, int colorSpace ) final;

        void destroySwapChain() final;

        void beginFrame( zp_uint64_t frameIndex ) final;

        void submit( zp_uint64_t frameIndex ) final;

        void present( zp_uint64_t frameIndex ) final;

        void waitForGPU() final;

#pragma region Create & Destroy Resources

        void createRenderPass( const RenderPassDesc* renderPassDesc, RenderPass* renderPass ) final;

        void destroyRenderPass( RenderPass* renderPass ) final;

        void createGraphicsPipeline( const GraphicsPipelineStateCreateDesc* graphicsPipelineStateCreateDesc, GraphicsPipelineState* graphicsPipelineState ) final;

        void destroyGraphicsPipeline( GraphicsPipelineState* graphicsPipelineState ) final;

        void createPipelineLayout( const PipelineLayoutDesc* pipelineLayoutDesc, PipelineLayout* pipelineLayout ) final;

        void destroyPipelineLayout( PipelineLayout* pipelineLayout ) final;

        void createBuffer( const GraphicsBufferDesc* graphicsBufferDesc, GraphicsBuffer* graphicsBuffer ) final;

        void destroyBuffer( GraphicsBuffer* graphicsBuffer ) final;

        void createShader( const ShaderDesc* shaderDesc, Shader* shader ) final;

        void destroyShader( Shader* shader ) final;

        void createTexture( const TextureCreateDesc* textureCreateDesc, Texture* texture ) final;

        void destroyTexture( Texture* texture ) final;

        void createSampler( const SamplerCreateDesc* samplerCreateDesc, Sampler* sampler ) final;

        void destroySampler( Sampler* sampler ) final;

        void mapBuffer( zp_size_t offset, zp_size_t size, GraphicsBuffer* graphicsBuffer, void** memory ) final;

        void unmapBuffer( GraphicsBuffer* graphicsBuffer ) final;

#pragma endregion

#pragma region Command Queue Operations

        CommandQueue* requestCommandQueue( RenderQueue queue, zp_uint64_t frameIndex ) final;

        void beginRenderPass( const RenderPass* renderPass, CommandQueue* commandQueue ) final;

        void nextSubpass( CommandQueue* commandQueue ) final;
        
        void endRenderPass( CommandQueue* commandQueue ) final;

        void bindPipeline( const GraphicsPipelineState* graphicsPipelineState, PipelineBindPoint bindPoint, CommandQueue* commandQueue ) final;

        void bindIndexBuffer( const GraphicsBuffer* graphicsBuffer, IndexBufferFormat indexBufferFormat, zp_size_t offset, CommandQueue* commandQueue ) final;

        void bindVertexBuffers( zp_uint32_t firstBinding, zp_uint32_t bindingCount, const GraphicsBuffer** graphicsBuffers, zp_size_t* offsets, CommandQueue* commandQueue ) final;

        void updateTexture( const TextureUpdateDesc* textureUpdateDesc, const Texture* dstTexture, CommandQueue* commandQueue ) final;

        void updateBuffer( const GraphicsBufferUpdateDesc* graphicsBufferUpdateDesc, const GraphicsBuffer* dstGraphicsBuffer, CommandQueue* commandQueue ) final;

#pragma endregion

#pragma region Draw Commands

        void draw( zp_uint32_t vertexCount, zp_uint32_t instanceCount, zp_uint32_t firstVertex, zp_uint32_t firstInstance, CommandQueue* commandQueue ) final;

        void drawIndexed( zp_uint32_t indexCount, zp_uint32_t instanceCount, zp_uint32_t firstIndex, zp_int32_t vertexOffset, zp_uint32_t firstInstance, CommandQueue* commandQueue ) final;

#pragma endregion

#pragma region Profiler Markers

        void beginEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue ) final;

        void endEventLabel( CommandQueue* commandQueue ) final;

        void markEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue ) final;

#pragma endregion


    private:
        struct QueueFamilies
        {
            zp_uint32_t graphicsFamily;
            zp_uint32_t transferFamily;
            zp_uint32_t computeFamily;
            zp_uint32_t presentFamily;
        };

        struct PerFrameData
        {
            VkSemaphore vkSwapChainAcquireSemaphore;
            VkSemaphore vkRenderFinishedSemaphore;
            VkFence vkInFlightFences;
            VkFence vkSwapChainImageAcquiredFence;
            zp_uint32_t swapChainImageIndex;

            VkDescriptorPool vkDescriptorPool;

#if ZP_USE_PROFILER
            VkQueryPool vkPipelineStatisticsQueryPool;
            VkQueryPool vkTimestampQueryPool;
#endif
            GraphicsBufferAllocator perFrameStagingBuffer;

            zp_size_t commandQueueCount;
            zp_size_t commandQueueCapacity;
            CommandQueue* commandQueues;
        };

        PerFrameData m_perFrameData[kBufferedFrameCount];

        VkInstance m_vkInstance;
        VkSurfaceKHR m_vkSurface;
        VkPhysicalDevice m_vkPhysicalDevice;
        VkDevice m_vkLocalDevice;

        VkQueue m_vkRenderQueues[4];

        VkSwapchainKHR m_vkSwapChain;
        VkRenderPass m_vkSwapChainRenderPass;

        VkDescriptorPool m_vkDescriptorPool;
        VkPipelineCache m_vkPipelineCache;

        VkCommandPool m_vkGraphicsCommandPool;
        VkCommandPool m_vkTransferCommandPool;
        VkCommandPool m_vkComputeCommandPool;

        VkFormat m_vkSwapChainFormat;
        VkColorSpaceKHR m_vkSwapChainColorSpace;
        VkExtent2D m_vkSwapChainExtent;

#if ZP_DEBUG
        VkDebugUtilsMessengerEXT m_vkDebugMessenger;
#endif

        Vector<VkImage> m_swapChainImages;
        Vector<VkImageView> m_swapChainImageViews;
        Vector<VkFramebuffer> m_swapChainFrameBuffers;
        Vector<VkFence> m_swapChainInFlightFences;

        GraphicsBuffer m_stagingBuffer;

        QueueFamilies m_queueFamilies;
    };
}

#endif //ZP_VULKANGRAPHICSDEVICE_H
