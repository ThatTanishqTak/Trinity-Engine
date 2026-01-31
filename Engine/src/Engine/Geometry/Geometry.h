#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Engine
{
    namespace Geometry
    {
        class Vertex
        {
            glm::vec3 Position;
            glm::vec3 Color;

            static VkVertexInputBindingDescription GetBindingDescription()
            {
                VkVertexInputBindingDescription l_Binding{};
                l_Binding.binding = 0;
                l_Binding.stride = sizeof(Vertex);
                l_Binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return l_Binding;
            }

            static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
            {
                std::array<VkVertexInputAttributeDescription, 2> l_Attributes{};

                l_Attributes[0].binding = 0;
                l_Attributes[0].location = 0;
                l_Attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                l_Attributes[0].offset = offsetof(Vertex, Position);

                l_Attributes[1].binding = 0;
                l_Attributes[1].location = 1;
                l_Attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                l_Attributes[1].offset = offsetof(Vertex, Color);

                return l_Attributes;
            }
        };

        static const std::array<Vertex, 3> s_TriangleVertices =
        {
            Vertex{ {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.2f, 0.2f } },
            Vertex{ {  0.5f, -0.5f, 0.0f }, { 0.2f, 1.0f, 0.2f } },
            Vertex{ { -0.5f, -0.5f, 0.0f }, { 0.2f, 0.2f, 1.0f } },
        };

        static const std::array<Vertex, 24> s_CubeVertices =
        {
            Vertex{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
            Vertex{ {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
            Vertex{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
            Vertex{ { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },

            Vertex{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
            Vertex{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
            Vertex{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
            Vertex{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },

            Vertex{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },
            Vertex{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },
            Vertex{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
            Vertex{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },

            Vertex{ {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
            Vertex{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
            Vertex{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },
            Vertex{ {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },

            Vertex{ { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
            Vertex{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
            Vertex{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },
            Vertex{ { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },

            Vertex{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
            Vertex{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
            Vertex{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f } },
            Vertex{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f } },
        };

        static const std::array<uint16_t, 36> s_CubeIndices =
        {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };
    }
}