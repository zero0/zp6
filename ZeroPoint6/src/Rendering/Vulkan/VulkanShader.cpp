//
// Created by phosg on 11/6/2021.
//

#include <vulkan/vulkan.h>

static VkResult CreateShader(
    VkDevice localDevice,
    const VkAllocationCallbacks* allocationCallbacks,
    void* data,
    size_t size,
    VkShaderModule& shaderModule)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = size;
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(data);

    const VkResult result = vkCreateShaderModule(
        localDevice,
        &shaderModuleCreateInfo,
        allocationCallbacks,
        &shaderModule);
    return result;
}

static void DestroyShader(
    VkDevice localDevice,
    const VkAllocationCallbacks* allocationCallbacks,
    VkShaderModule shaderModule)
{
    vkDestroyShaderModule(localDevice, shaderModule, allocationCallbacks);
}

static VkResult CreateVertexShader(
    VkDevice localDevice,
    const VkAllocationCallbacks* allocationCallbacks,
    VkShaderModule shaderModule,
    const char* name
        )
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = name == nullptr ? "main" : name;
    shaderStageCreateInfo.pSpecializationInfo = nullptr;

    return VK_SUCCESS;
}