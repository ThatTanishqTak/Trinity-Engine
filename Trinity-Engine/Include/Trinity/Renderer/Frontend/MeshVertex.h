#pragma once

#include <glm/glm.hpp>

#include <Trinity/Renderer/RHI/Pipeline.h>

namespace Trinity
{
    struct MeshVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec2 UV;

        static VertexLayout GetLayout()
        {
            VertexLayout l_Layout;
            l_Layout.Stride = sizeof(MeshVertex);
            l_Layout.Attributes = { { 0, offsetof(MeshVertex, Position), Format::RGB32_SFLOAT }, { 1, offsetof(MeshVertex, Normal), Format::RGB32_SFLOAT }, { 2, offsetof(MeshVertex, Tangent), Format::RGB32_SFLOAT }, { 3, offsetof(MeshVertex, UV), Format::RG32_SFLOAT } };

            return l_Layout;
        }
    };
}