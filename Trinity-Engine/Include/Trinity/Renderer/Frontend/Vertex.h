#pragma once

#include <glm/glm.hpp>

#include <Trinity/Renderer/RHI/Pipeline.h>

namespace Trinity
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Color;

        static VertexLayout GetLayout()
        {
            VertexLayout l_Layout;
            l_Layout.Stride = sizeof(Vertex);
            l_Layout.Attributes = { { 0, offsetof(Vertex, Position), Format::RGB32_SFLOAT }, { 1, offsetof(Vertex, Color), Format::RGB32_SFLOAT } };

            return l_Layout;
        }
    };
}