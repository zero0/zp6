//
// Created by phosg on 2/3/2022.
//

#ifndef ZP_VULKANGRAPHICSDEVICE_H
#define ZP_VULKANGRAPHICSDEVICE_H

#include "Core/Types.h"
#include "Core/Common.h"

#include "Rendering/GraphicsDevice.h"

namespace zp
{
    namespace internal
    {
        GraphicsDevice* CreateVulkanGraphicsDevice( MemoryLabel memoryLabel, const GraphicsDeviceDesc& desc );

        void DestroyVulkanGraphicsDevice( GraphicsDevice* graphicsDevice );
    }


#if 0
    class VulkanGraphicsDevice final : public GraphicsDevice
    {
    ZP_NONCOPYABLE( VulkanGraphicsDevice );

    public:
        VulkanGraphicsDevice( MemoryLabel memoryLabel, const GraphicsDeviceDesc& graphicsDeviceDesc );

        ~VulkanGraphicsDevice();

        void createSwapchain( zp_handle_t windowHandle, zp_uint32_t width, zp_uint32_t height, int displayFormat, ColorSpace colorSpace ) final;

        void resizeSwapchain( zp_uint32_t width, zp_uint32_t height ) final;

        void destroySwapchain() final;

        void createPerFrameData() final;

        void destroyPerFrameData() final;

        void beginFrame() final;

        void submit() final;

        void present() final;

        void waitForGPU() final;

#pragma region Create & Destroy Resources

        void createRenderPass( const RenderPassDesc* renderPassDesc, RenderPass* renderPass ) final;

        void destroyRenderPass( RenderPass* renderPass ) final;

        void createGraphicsPipeline( const GraphicsPipelineStateCreateDesc* graphicsPipelineStateCreateDesc, GraphicsPipelineState* graphicsPipelineState ) final;

        void destroyGraphicsPipeline( GraphicsPipelineState* graphicsPipelineState ) final;

        void createPipelineLayout( const PipelineLayoutDesc* pipelineLayoutDesc, PipelineLayout* pipelineLayout ) final;

        void destroyPipelineLayout( PipelineLayout* pipelineLayout ) final;

        void createBuffer( const GraphicsBufferDesc& graphicsBufferDesc, GraphicsBuffer* graphicsBuffer ) final;

        void destroyBuffer( GraphicsBuffer* graphicsBuffer ) final;

        void createShader( const ShaderDesc& shaderDesc, Shader* shader ) final;

        void destroyShader( Shader* shader ) final;

        void createTexture( const TextureCreateDesc* textureCreateDesc, Texture* texture ) final;

        void destroyTexture( Texture* texture ) final;

        void createSampler( const SamplerCreateDesc* samplerCreateDesc, Sampler* sampler ) final;

        void destroySampler( Sampler* sampler ) final;

        void mapBuffer( zp_size_t offset, zp_size_t size, const GraphicsBuffer& graphicsBuffer, void** memory ) final;

        void unmapBuffer( const GraphicsBuffer& graphicsBuffer ) final;

#pragma endregion

#pragma region Command Queue Operations

        CommandQueue* requestCommandQueue( RenderQueue queue ) final;

        CommandQueue* requestCommandQueue( CommandQueue* parentCommandQueue ) final;

        void releaseCommandQueue( CommandQueue* commandQueue ) final;

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

        void drawIndirect( const GraphicsBuffer& dstbuffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue ) final;

        void drawIndexed( zp_uint32_t indexCount, zp_uint32_t instanceCount, zp_uint32_t firstIndex, zp_int32_t vertexOffset, zp_uint32_t firstInstance, CommandQueue* commandQueue ) final;

        void drawIndexedIndirect( const GraphicsBuffer& dstbuffer, zp_uint32_t drawCount, zp_uint32_t stride, CommandQueue* commandQueue ) final;

#pragma endregion

#pragma region Compute Dispatch Commands

        void dispatch( zp_uint32_t groupCountX, zp_uint32_t groupCountY, zp_uint32_t groupCountZ, CommandQueue* commandQueue ) final;

        void dispatchIndirect( const GraphicsBuffer& dstbuffer, CommandQueue* commandQueue ) final;

#pragma endregion

#pragma region Profiler Markers

        void beginEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue ) final;

        void endEventLabel( CommandQueue* commandQueue ) final;

        void markEventLabel( const char* eventLabel, const Color& color, CommandQueue* commandQueue ) final;

#pragma endregion

    private:
#if 0
        VkCommandPool getCommandPool( CommandQueue* commandQueue );

        VkDescriptorSetLayout getDescriptorSetLayout( const VkDescriptorSetLayoutCreateInfo& createInfo );

        void processDelayedDestroy();

        void destroyAllDelayedDestroy();

        void rebuildSwapchain();

        struct QueueFamilyIndices
        {
            zp_uint32_t graphicsQueue;
            zp_uint32_t transferQueue;
            zp_uint32_t computeQueue;
            zp_uint32_t presentQueue;
        };

        struct PerFrameData
        {
            VkSemaphore vkSwapChainAcquireSemaphore;
            VkSemaphore vkRenderFinishedSemaphore;
            VkFence vkInFlightFence;
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

        struct SwapchainData
        {
            zp_handle_t windowHandle;

            VkSwapchainKHR vkSwapchain;
            VkRenderPass vkSwapchainDefaultRenderPass;
            VkFormat vkSwapChainFormat;

            VkColorSpaceKHR vkSwapchainColorSpace;
            VkExtent2D m_vkSwapchainExtent;

            zp_uint32_t swapchainImageCount;
            FixedArray<VkImage, kMaxBufferedFrameCount> swapchainImages;
            FixedArray<VkImageView, kMaxBufferedFrameCount> swapchainImageViews;
            FixedArray<VkFramebuffer, kMaxBufferedFrameCount> swapchainFrameBuffers;
        };

        PerFrameData& getCurrentFrameData();

        const PerFrameData& getCurrentFrameData() const;

        PerFrameData& getFrameData( zp_uint64_t frameCount );

        PerFrameData m_perFrameData[kMaxBufferedFrameCount];

        SwapchainData m_swapchainData;

        VkInstance m_vkInstance;
        VkSurfaceKHR m_vkSurface;
        VkPhysicalDevice m_vkPhysicalDevice;
        VkDevice m_vkLocalDevice;
        VkPhysicalDeviceMemoryProperties m_vkPhysicalDeviceMemoryProperties{};

        VkAllocationCallbacks m_vkAllocationCallbacks;

        VkQueue m_vkQueues[RenderQueue_Count];

        VkPipelineCache m_vkPipelineCache;
        VkDescriptorPool m_vkDescriptorPool;

        VkCommandPool* m_vkCommandPools;
        zp_size_t m_commandPoolCount;

#if ZP_DEBUG
        VkDebugUtilsMessengerEXT m_vkDebugMessenger;
#endif

        Map<zp_hash128_t, VkDescriptorSetLayout, zp_hash128_t> m_descriptorSetLayoutCache;
        Map<zp_hash128_t, VkSampler, zp_hash128_t> m_samplerCache;

        Vector<DelayedDestroy> m_delayedDestroy;

        GraphicsBuffer m_stagingBuffer;

        QueueFamilyIndices m_queueFamilies;

        zp_uint64_t m_currentFrameIndex;
#endif
    public:
        const MemoryLabel memoryLabel;
    };
#endif
}

#endif //ZP_VULKANGRAPHICSDEVICE_H
