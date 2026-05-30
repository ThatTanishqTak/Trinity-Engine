#include <Trinity/Renderer/Backends/Vulkan/VulkanUtilities.h>

namespace Trinity::VulkanUtilities
{
    VkFormat ToVkFormat(Format format)
    {
        switch (format)
        {
            case Format::R8_UNORM: return VK_FORMAT_R8_UNORM;
            case Format::RG8_UNORM: return VK_FORMAT_R8G8_UNORM;
            case Format::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case Format::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case Format::BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case Format::BGRA8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case Format::R16_SFLOAT: return VK_FORMAT_R16_SFLOAT;
            case Format::RG16_SFLOAT: return VK_FORMAT_R16G16_SFLOAT;
            case Format::RGBA16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case Format::R32_SFLOAT: return VK_FORMAT_R32_SFLOAT;
            case Format::RG32_SFLOAT: return VK_FORMAT_R32G32_SFLOAT;
            case Format::RGB32_SFLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
            case Format::RGBA32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Format::R32_UINT: return VK_FORMAT_R32_UINT;
            case Format::R16_UINT: return VK_FORMAT_R16_UINT;
            case Format::D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
            case Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
            case Format::D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage)
    {
        VkBufferUsageFlags l_Flags = 0;

        if (HasUsage(usage, BufferUsage::Vertex))
        {
            l_Flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        
        if (HasUsage(usage, BufferUsage::Index))
        {
            l_Flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        
        if (HasUsage(usage, BufferUsage::Uniform))
        {
            l_Flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        
        if (HasUsage(usage, BufferUsage::Storage))
        {
            l_Flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        
        if (HasUsage(usage, BufferUsage::TransferSource))
        {
            l_Flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (HasUsage(usage, BufferUsage::TransferDestination))
        {
            l_Flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        return l_Flags;
    }

    VkImageUsageFlags ToVkImageUsage(TextureUsage usage)
    {
        VkImageUsageFlags l_Flags = 0;

        if (HasUsage(usage, TextureUsage::Sampled))
        {
            l_Flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        
        if (HasUsage(usage, TextureUsage::Storage))
        {
            l_Flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (HasUsage(usage, TextureUsage::RenderTarget))
        {
            l_Flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        if (HasUsage(usage, TextureUsage::DepthStencil))
        {
            l_Flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (HasUsage(usage, TextureUsage::TransferSource))
        {
            l_Flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (HasUsage(usage, TextureUsage::TransferDestination))
        {
            l_Flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        return l_Flags;
    }

    VmaAllocationParameters ToVmaAllocation(MemoryUsage memory)
    {
        VmaAllocationParameters l_Parameters;

        switch (memory)
        {
            case MemoryUsage::CpuToGpu:
                l_Parameters.Flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case MemoryUsage::GpuToCpu:
                l_Parameters.Flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;

            case MemoryUsage::GpuOnly:
            default:
                l_Parameters.Flags = 0;
                break;
        }

        return l_Parameters;
    }

    VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
            case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkCullModeFlags ToVkCullMode(CullMode mode)
    {
        switch (mode)
        {
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            default: return VK_CULL_MODE_NONE;
        }
    }

    VkFrontFace ToVkFrontFace(FrontFace face)
    {
        return face == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    VkCompareOp ToVkCompareOp(CompareOp op)
    {
        switch (op)
        {
            case CompareOp::Never: return VK_COMPARE_OP_NEVER;
            case CompareOp::Less: return VK_COMPARE_OP_LESS;
            case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
            case CompareOp::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
            case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
            default: return VK_COMPARE_OP_LESS;
        }
    }

    VkShaderStageFlags ToVkShaderStages(ShaderStage stages)
    {
        VkShaderStageFlags l_Flags = 0;

        if (HasStage(stages, ShaderStage::Vertex))
        {
            l_Flags |= VK_SHADER_STAGE_VERTEX_BIT;
        }

        if (HasStage(stages, ShaderStage::Fragment))
        {
            l_Flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        if (HasStage(stages, ShaderStage::Compute))
        {
            l_Flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        }

        if (HasStage(stages, ShaderStage::Geometry))
        {
            l_Flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }

        if (HasStage(stages, ShaderStage::TessControl))
        {
            l_Flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }

        if (HasStage(stages, ShaderStage::TessEvaluation))
        {
            l_Flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }

        return l_Flags;
    }
}