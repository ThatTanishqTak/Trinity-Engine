#include "Engine/Renderer/Vulkan/VulkanPipeline.h"

#include "Engine/Renderer/Vulkan/VulkanDebugUtils.h"
#include "Engine/Renderer/Vulkan/VulkanDevice.h"
#include "Engine/Utilities/Utilities.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace Engine
{
    void VulkanPipeline::Initialize(VulkanDevice& device, VkRenderPass renderPass, const GraphicsPipelineDescription& description, std::span<const VkDescriptorSetLayout> descriptorSetLayouts)
    {
        Shutdown(device);

        m_RenderPass = renderPass;
        m_Extent = description.Extent;
        m_VertexShaderPath = description.VertexShaderPath;
        m_FragmentShaderPath = description.FragmentShaderPath;
        m_PipelineCachePath = description.PipelineCachePath;
        m_DescriptorSetLayouts.assign(descriptorSetLayouts.begin(), descriptorSetLayouts.end());

        CreatePipelineCache(device, m_PipelineCachePath);
        const VulkanDebugUtils* l_DebugUtils = device.GetDebugUtils();
        if (l_DebugUtils && m_PipelineCache)
        {
            const std::string l_BaseName = description.DebugName.empty() ? "GraphicsPipeline" : description.DebugName;
            const std::string l_CacheName = l_BaseName + "_PipelineCache";
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_PIPELINE_CACHE, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_PipelineCache)), l_CacheName.c_str());
        }

        VkShaderModule l_VertModule = VK_NULL_HANDLE;
        VkShaderModule l_FragModule = VK_NULL_HANDLE;

        try
        {
            l_VertModule = CreateShaderModule(device, m_VertexShaderPath);
            l_FragModule = CreateShaderModule(device, m_FragmentShaderPath);
        }
        catch (...)
        {
            // Keep cache alive even if shaders fail
            Shutdown(device);

            throw;
        }

        VkPipelineShaderStageCreateInfo l_VertStage{};
        l_VertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_VertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        l_VertStage.module = l_VertModule;
        l_VertStage.pName = "main";

        VkPipelineShaderStageCreateInfo l_FragStage{};
        l_FragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        l_FragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        l_FragStage.module = l_FragModule;
        l_FragStage.pName = "main";

        VkPipelineShaderStageCreateInfo l_ShaderStages[] = { l_VertStage, l_FragStage };

        // No vertex buffers (hello triangle using gl_VertexIndex in shader).
        VkPipelineVertexInputStateCreateInfo l_VertexInput{};
        l_VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo l_InputAssembly{};
        l_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        l_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        l_InputAssembly.primitiveRestartEnable = VK_FALSE;

        // Dynamic viewport/scissor.
        VkPipelineViewportStateCreateInfo l_ViewportState{};
        l_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        l_ViewportState.viewportCount = 1;
        l_ViewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo l_Raster{};
        l_Raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        l_Raster.depthClampEnable = VK_FALSE;
        l_Raster.rasterizerDiscardEnable = VK_FALSE;
        l_Raster.polygonMode = VK_POLYGON_MODE_FILL;
        l_Raster.lineWidth = 1.0f;
        l_Raster.cullMode = description.CullMode;
        l_Raster.frontFace = description.FrontFace;
        l_Raster.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo l_MSAA{};
        l_MSAA.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        l_MSAA.sampleShadingEnable = VK_FALSE;
        l_MSAA.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState l_ColorBlendAttachment{};
        l_ColorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        l_ColorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo l_ColorBlend{};
        l_ColorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        l_ColorBlend.logicOpEnable = VK_FALSE;
        l_ColorBlend.attachmentCount = 1;
        l_ColorBlend.pAttachments = &l_ColorBlendAttachment;

        VkDynamicState l_Dynamics[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo l_DynamicState{};
        l_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        l_DynamicState.dynamicStateCount = (uint32_t)(sizeof(l_Dynamics) / sizeof(l_Dynamics[0]));
        l_DynamicState.pDynamicStates = l_Dynamics;

        VkPipelineLayoutCreateInfo l_LayoutInfo{};
        l_LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        l_LayoutInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptorSetLayouts.size());
        l_LayoutInfo.pSetLayouts = m_DescriptorSetLayouts.empty() ? nullptr : m_DescriptorSetLayouts.data();

        VkPushConstantRange l_PushConstantRange{};
        if (description.PushConstantSize > 0)
        {
            l_PushConstantRange.stageFlags = description.PushConstantStages;
            l_PushConstantRange.offset = 0;
            l_PushConstantRange.size = description.PushConstantSize;

            l_LayoutInfo.pushConstantRangeCount = 1;
            l_LayoutInfo.pPushConstantRanges = &l_PushConstantRange;
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkCreatePipelineLayout(device.GetDevice(), &l_LayoutInfo, nullptr, &m_PipelineLayout), "vkCreatePipelineLayout");
        if (l_DebugUtils && m_PipelineLayout)
        {
            const std::string l_BaseName = description.DebugName.empty() ? "GraphicsPipeline" : description.DebugName;
            const std::string l_LayoutName = l_BaseName + "_PipelineLayout";
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_PipelineLayout)), l_LayoutName.c_str());
        }

        VkGraphicsPipelineCreateInfo l_PipelineInfo{};
        l_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        l_PipelineInfo.stageCount = 2;
        l_PipelineInfo.pStages = l_ShaderStages;
        l_PipelineInfo.pVertexInputState = &l_VertexInput;
        l_PipelineInfo.pInputAssemblyState = &l_InputAssembly;
        l_PipelineInfo.pViewportState = &l_ViewportState;
        l_PipelineInfo.pRasterizationState = &l_Raster;
        l_PipelineInfo.pMultisampleState = &l_MSAA;
        l_PipelineInfo.pColorBlendState = &l_ColorBlend;
        l_PipelineInfo.pDynamicState = &l_DynamicState;
        l_PipelineInfo.layout = m_PipelineLayout;
        l_PipelineInfo.renderPass = m_RenderPass;
        l_PipelineInfo.subpass = 0;

        Utilities::VulkanUtilities::VKCheckStrict(vkCreateGraphicsPipelines(device.GetDevice(), m_PipelineCache, 1, &l_PipelineInfo, nullptr, &m_Pipeline), "vkCreateGraphicsPipelines");
        if (l_DebugUtils && m_Pipeline)
        {
            const std::string l_BaseName = description.DebugName.empty() ? "GraphicsPipeline" : description.DebugName;
            const std::string l_PipelineName = l_BaseName + "_Pipeline";
            l_DebugUtils->SetObjectName(device.GetDevice(), VK_OBJECT_TYPE_PIPELINE, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_Pipeline)), l_PipelineName.c_str());
        }

        vkDestroyShaderModule(device.GetDevice(), l_VertModule, nullptr);
        vkDestroyShaderModule(device.GetDevice(), l_FragModule, nullptr);

        m_Initialized = true;
    }

    void VulkanPipeline::Shutdown(VulkanDevice& device)
    {
        Shutdown(device, {});
    }

    void VulkanPipeline::Shutdown(VulkanDevice & device, const std::function<void(std::function<void()>&&)>&submitResourceFree)
    {
        if (!device.GetDevice())
        {
            m_Initialized = false;

            return;
        }

        // Try saving cache before destroying it.
        if (m_PipelineCache != VK_NULL_HANDLE && !m_PipelineCachePath.empty())
        {
            try
            {
                SavePipelineCache(device, m_PipelineCachePath);
            }
            catch (...)
            {
                /* best-effort */
            }
        }

        VkPipeline l_Pipeline = m_Pipeline;
        VkPipelineLayout l_PipelineLayout = m_PipelineLayout;
        VkPipelineCache l_PipelineCache = m_PipelineCache;
        VulkanDevice* l_Device = &device;

        m_Pipeline = VK_NULL_HANDLE;
        m_PipelineLayout = VK_NULL_HANDLE;
        m_PipelineCache = VK_NULL_HANDLE;

        m_RenderPass = VK_NULL_HANDLE;
        m_Extent = {};
        m_DescriptorSetLayouts.clear();
        m_VertexShaderPath.clear();
        m_FragmentShaderPath.clear();
        m_PipelineCachePath.clear();

        m_Initialized = false;
        if (submitResourceFree)
        {
            submitResourceFree([l_Device, l_Pipeline, l_PipelineLayout, l_PipelineCache]() mutable
            {
                if (!l_Device || !l_Device->GetDevice())
                {
                    return;
                }

                if (l_Pipeline)
                {
                    vkDestroyPipeline(l_Device->GetDevice(), l_Pipeline, nullptr);
                }

                if (l_PipelineLayout)
                {
                    vkDestroyPipelineLayout(l_Device->GetDevice(), l_PipelineLayout, nullptr);
                }

                if (l_PipelineCache)
                {
                    vkDestroyPipelineCache(l_Device->GetDevice(), l_PipelineCache, nullptr);
                }
            });

            return;
        }

        if (l_Pipeline)
        {
            vkDestroyPipeline(device.GetDevice(), l_Pipeline, nullptr);
        }

        if (l_PipelineLayout)
        {
            vkDestroyPipelineLayout(device.GetDevice(), l_PipelineLayout, nullptr);
        }

        if (l_PipelineCache)
        {
            vkDestroyPipelineCache(device.GetDevice(), l_PipelineCache, nullptr);
        }
    }

    void VulkanPipeline::Recreate(VulkanDevice& device, VkRenderPass renderPass, const GraphicsPipelineDescription& description, std::span<const VkDescriptorSetLayout> descriptorSetLayouts)
    {
        Initialize(device, renderPass, description, descriptorSetLayouts);
    }

    VkShaderModule VulkanPipeline::CreateShaderModule(VulkanDevice& device, const std::string& path)
    {
        const std::vector<char> l_Code = Utilities::FileManagement::LoadFromFile(path);
        if (l_Code.empty())
        {
            TR_CORE_CRITICAL("Shader file is empty: {}", path);

            std::abort();
        }

        VkShaderModuleCreateInfo l_CreateInfo{};
        l_CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        l_CreateInfo.codeSize = l_Code.size();
        l_CreateInfo.pCode = reinterpret_cast<const uint32_t*>(l_Code.data());

        VkShaderModule l_Module = VK_NULL_HANDLE;
        Utilities::VulkanUtilities::VKCheckStrict(vkCreateShaderModule(device.GetDevice(), &l_CreateInfo, nullptr, &l_Module), "vkCreateShaderModule");

        return l_Module;
    }

    void VulkanPipeline::CreatePipelineCache(VulkanDevice& device, const std::string& cachePath)
    {
        std::vector<char> l_CacheData;
        if (!cachePath.empty())
        {
            std::ifstream l_In(cachePath, std::ios::binary | std::ios::ate);
            if (l_In.is_open())
            {
                const std::streamsize l_Size = l_In.tellg();
                if (l_Size > 0)
                {
                    l_CacheData.resize((size_t)l_Size);
                    l_In.seekg(0);
                    l_In.read(l_CacheData.data(), l_Size);
                }
                l_In.close();
            }
        }

        VkPipelineCacheCreateInfo l_Info{};
        l_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        if (!l_CacheData.empty())
        {
            l_Info.initialDataSize = (size_t)l_CacheData.size();
            l_Info.pInitialData = l_CacheData.data();
        }

        Utilities::VulkanUtilities::VKCheckStrict(vkCreatePipelineCache(device.GetDevice(), &l_Info, nullptr, &m_PipelineCache), "vkCreatePipelineCache");
    }

    void VulkanPipeline::SavePipelineCache(VulkanDevice& device, const std::string& cachePath)
    {
        if (cachePath.empty() || m_PipelineCache == VK_NULL_HANDLE)
        {
            return;
        }

        size_t l_Size = 0;
        Utilities::VulkanUtilities::VKCheckStrict(vkGetPipelineCacheData(device.GetDevice(), m_PipelineCache, &l_Size, nullptr), "vkGetPipelineCacheData(size)");

        if (l_Size == 0)
        {
            return;
        }

        std::vector<uint8_t> l_Data(l_Size);
        Utilities::VulkanUtilities::VKCheckStrict(vkGetPipelineCacheData(device.GetDevice(), m_PipelineCache, &l_Size, l_Data.data()), "vkGetPipelineCacheData(data)");

        std::ofstream l_Out(cachePath, std::ios::binary | std::ios::trunc);
        if (!l_Out.is_open())
        {
            return; // best-effort
        }

        l_Out.write(reinterpret_cast<const char*>(l_Data.data()), (std::streamsize)l_Data.size());
        l_Out.close();
    }
}