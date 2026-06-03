#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <Trinity/Renderer/Frontend/MeshVertex.h>

namespace Trinity
{
    struct Submesh
    {
        uint32_t FirstIndex = 0;
        uint32_t IndexCount = 0;
        uint32_t BaseVertex = 0;
        uint32_t MaterialIndex = 0;
        std::string Name;
    };

    struct MaterialSlot
    {
        std::string Name;
        std::string BaseColorTexturePath;
        glm::vec4 BaseColorFactor = glm::vec4(1.0f);
    };

    struct MeshImportDiagnostics
    {
        std::string SourcePath;
        std::string SourceFormat;
        bool GeneratedNormals = false;
        bool GeneratedTangents = false;
        std::vector<std::string> Warnings;
    };

    struct MeshData
    {
        std::vector<MeshVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<Submesh> Submeshes;
        std::vector<MaterialSlot> MaterialSlots;
        MeshImportDiagnostics Diagnostics;
    };
}