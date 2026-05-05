#pragma once

#include "Trinity/Renderer/Resources/Buffer.h"
#include "Trinity/Renderer/Resources/Pipeline.h"
#include "Trinity/Renderer/Resources/Sampler.h"
#include "Trinity/Renderer/Resources/Shader.h"
#include "Trinity/Renderer/Resources/Texture.h"

#include "Trinity/Utilities/Log.h"

#include <cstdlib>

namespace Trinity
{
    namespace VulkanUtilities
    {
        inline void VKCheck(VkResult result, const char* what)
        {
            if (result != VK_SUCCESS)
            {
                std::abort();
            }
        }

        inline VkFormat ToVkFormat(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::R8:
                    return VK_FORMAT_R8_UNORM;
                case TextureFormat::RG8:
                    return VK_FORMAT_R8G8_UNORM;
                case TextureFormat::RGBA8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGBA8_SRGB:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case TextureFormat::BGRA8:
                    return VK_FORMAT_B8G8R8A8_UNORM;
                case TextureFormat::BGRA8_SRGB:
                    return VK_FORMAT_B8G8R8A8_SRGB;

                case TextureFormat::R16F:
                    return VK_FORMAT_R16_SFLOAT;
                case TextureFormat::RG16F:
                    return VK_FORMAT_R16G16_SFLOAT;
                case TextureFormat::RGBA16F:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;

                case TextureFormat::R32F:
                    return VK_FORMAT_R32_SFLOAT;
                case TextureFormat::RG32F:
                    return VK_FORMAT_R32G32_SFLOAT;
                case TextureFormat::RGBA32F:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;

                case TextureFormat::R11G11B10F:
                    return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                case TextureFormat::RGB10A2:
                    return VK_FORMAT_A2B10G10R10_UNORM_PACK32;

                case TextureFormat::Depth16Unorm:
                    return VK_FORMAT_D16_UNORM;
                case TextureFormat::Depth32F:
                    return VK_FORMAT_D32_SFLOAT;
                case TextureFormat::Depth24Stencil8:
                    return VK_FORMAT_D24_UNORM_S8_UINT;
                case TextureFormat::Depth32FStencil8:
                    return VK_FORMAT_D32_SFLOAT_S8_UINT;

                case TextureFormat::BC1:
                    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                case TextureFormat::BC2:
                    return VK_FORMAT_BC2_UNORM_BLOCK;
                case TextureFormat::BC3:
                    return VK_FORMAT_BC3_UNORM_BLOCK;
                case TextureFormat::BC4:
                    return VK_FORMAT_BC4_UNORM_BLOCK;
                case TextureFormat::BC5:
                    return VK_FORMAT_BC5_UNORM_BLOCK;
                case TextureFormat::BC6H:
                    return VK_FORMAT_BC6H_UFLOAT_BLOCK;
                case TextureFormat::BC7:
                    return VK_FORMAT_BC7_UNORM_BLOCK;

                case TextureFormat::ASTC_4x4:
                    return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;

                default:
                    return VK_FORMAT_UNDEFINED;
            }
        }

        inline bool IsDepthFormat(TextureFormat format)
        {
            return format == TextureFormat::Depth16Unorm || format == TextureFormat::Depth32F || format == TextureFormat::Depth24Stencil8 || format == TextureFormat::Depth32FStencil8;
        }

        inline bool IsBlockCompressedFormat(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::BC1:
                case TextureFormat::BC2:
                case TextureFormat::BC3:
                case TextureFormat::BC4:
                case TextureFormat::BC5:
                case TextureFormat::BC6H:
                case TextureFormat::BC7:
                case TextureFormat::ASTC_4x4:
                    return true;
                default:
                    return false;
            }
        }

        inline VkImageUsageFlags ToVkImageUsage(TextureUsage usage)
        {
            VkImageUsageFlags l_Flags = 0;
            if (usage & TextureUsage::Sampled)
            {
                l_Flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }

            if (usage & TextureUsage::Storage)
            {
                l_Flags |= VK_IMAGE_USAGE_STORAGE_BIT;
            }

            if (usage & TextureUsage::ColorAttachment)
            {
                l_Flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }

            if (usage & TextureUsage::DepthStencilAttachment)
            {
                l_Flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }

            if (usage & TextureUsage::TransferSource)
            {
                l_Flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }

            if (usage & TextureUsage::TransferDestination)
            {
                l_Flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

            if (usage & TextureUsage::InputAttachment)
            {
                l_Flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }

            return l_Flags;
        }

        inline VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage)
        {
            VkBufferUsageFlags l_Flags = 0;
            if (usage & BufferUsage::VertexBuffer)
            {
                l_Flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }

            if (usage & BufferUsage::IndexBuffer)
            {
                l_Flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }

            if (usage & BufferUsage::UniformBuffer)
            {
                l_Flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }

            if (usage & BufferUsage::StorageBuffer)
            {
                l_Flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }

            if (usage & BufferUsage::TransferSource)
            {
                l_Flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }

            if (usage & BufferUsage::TransferDestination)
            {
                l_Flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }

            if (usage & BufferUsage::IndirectBuffer)
            {
                l_Flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            if (usage & BufferUsage::BufferDeviceAddress)
            {
                l_Flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            }

            return l_Flags;
        }

        inline VkFilter ToVkFilter(SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::Nearest:
                    return VK_FILTER_NEAREST;
                case SamplerFilter::Linear:
                    return VK_FILTER_LINEAR;
                default:
                    return VK_FILTER_LINEAR;
            }
        }

        inline VkSamplerAddressMode ToVkAddressMode(SamplerAddressMode mode)
        {
            switch (mode)
            {
                case SamplerAddressMode::Repeat:
                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case SamplerAddressMode::MirroredRepeat:
                    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                case SamplerAddressMode::ClampToEdge:
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case SamplerAddressMode::ClampToBorder:
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                default:
                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
        }

        inline VkSamplerMipmapMode ToVkMipmapMode(SamplerMipmapMode mode)
        {
            switch (mode)
            {
                case SamplerMipmapMode::Nearest:
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
                case SamplerMipmapMode::Linear:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
                default:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
        }

        inline VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology)
        {
            switch (topology)
            {
                case PrimitiveTopology::TriangleList:
                    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case PrimitiveTopology::TriangleStrip:
                    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                case PrimitiveTopology::LineList:
                    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case PrimitiveTopology::LineStrip:
                    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                case PrimitiveTopology::PointList:
                    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                default:
                    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            }
        }

        inline VkCullModeFlags ToVkCullMode(CullMode mode)
        {
            switch (mode)
            {
                case CullMode::None:
                    return VK_CULL_MODE_NONE;
                case CullMode::Front:
                    return VK_CULL_MODE_FRONT_BIT;
                case CullMode::Back:
                    return VK_CULL_MODE_BACK_BIT;
                case CullMode::FrontAndBack:
                    return VK_CULL_MODE_FRONT_AND_BACK;
                default:
                    return VK_CULL_MODE_NONE;
            }
        }

        inline VkCompareOp ToVkCompareOp(DepthCompareOp op)
        {
            switch (op)
            {
                case DepthCompareOp::Never:
                    return VK_COMPARE_OP_NEVER;
                case DepthCompareOp::Less:
                    return VK_COMPARE_OP_LESS;
                case DepthCompareOp::Equal:
                    return VK_COMPARE_OP_EQUAL;
                case DepthCompareOp::LessOrEqual:
                    return VK_COMPARE_OP_LESS_OR_EQUAL;
                case DepthCompareOp::Greater:
                    return VK_COMPARE_OP_GREATER;
                case DepthCompareOp::NotEqual:
                    return VK_COMPARE_OP_NOT_EQUAL;
                case DepthCompareOp::GreaterOrEqual:
                    return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case DepthCompareOp::Always:
                    return VK_COMPARE_OP_ALWAYS;
                default:
                    return VK_COMPARE_OP_LESS;
            }
        }

        inline VkFormat ToVkVertexFormat(VertexAttributeFormat format)
        {
            switch (format)
            {
                case VertexAttributeFormat::Float:
                    return VK_FORMAT_R32_SFLOAT;
                case VertexAttributeFormat::Float2:
                    return VK_FORMAT_R32G32_SFLOAT;
                case VertexAttributeFormat::Float3:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case VertexAttributeFormat::Float4:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case VertexAttributeFormat::Int:
                    return VK_FORMAT_R32_SINT;
                case VertexAttributeFormat::Int2:
                    return VK_FORMAT_R32G32_SINT;
                case VertexAttributeFormat::Int3:
                    return VK_FORMAT_R32G32B32_SINT;
                case VertexAttributeFormat::Int4:
                    return VK_FORMAT_R32G32B32A32_SINT;
                case VertexAttributeFormat::UInt:
                    return VK_FORMAT_R32_UINT;
                case VertexAttributeFormat::UInt2:
                    return VK_FORMAT_R32G32_UINT;
                case VertexAttributeFormat::UInt3:
                    return VK_FORMAT_R32G32B32_UINT;
                case VertexAttributeFormat::UInt4:
                    return VK_FORMAT_R32G32B32A32_UINT;
                default:
                    return VK_FORMAT_R32G32B32_SFLOAT;
            }
        }

        inline VkShaderStageFlagBits ToVkShaderStage(ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::Vertex:
                    return VK_SHADER_STAGE_VERTEX_BIT;
                case ShaderStage::Fragment:
                    return VK_SHADER_STAGE_FRAGMENT_BIT;
                case ShaderStage::Compute:
                    return VK_SHADER_STAGE_COMPUTE_BIT;
                case ShaderStage::Geometry:
                    return VK_SHADER_STAGE_GEOMETRY_BIT;
                case ShaderStage::TessControl:
                    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                case ShaderStage::TessEval:
                    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                case ShaderStage::Task:
                    return VK_SHADER_STAGE_TASK_BIT_EXT;
                case ShaderStage::Mesh:
                    return VK_SHADER_STAGE_MESH_BIT_EXT;
                default:
                    return VK_SHADER_STAGE_VERTEX_BIT;
            }
        }

        inline VkImageAspectFlags GetAspectFlags(TextureFormat format)
        {
            if (format == TextureFormat::Depth32F)
            {
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            if (format == TextureFormat::Depth24Stencil8)
            {
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
}