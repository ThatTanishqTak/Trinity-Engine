#include "Trinity/Asset/Importers/MeshImporter.h"

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Utilities/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <cstdlib>
#include <filesystem>
#include <vector>

namespace Trinity
{
    std::shared_ptr<Mesh> MeshImporter::Import(const std::filesystem::path& path)
    {
        Assimp::Importer l_Importer;

        const aiScene* l_Scene = l_Importer.ReadFile(path.string(),
            aiProcess_Triangulate         |
            aiProcess_GenNormals          |
            aiProcess_CalcTangentSpace    |
            aiProcess_FlipUVs             |
            aiProcess_JoinIdenticalVertices |
            aiProcess_OptimizeMeshes
        );

        if (!l_Scene || (l_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !l_Scene->mRootNode)
        {
            TR_CORE_ERROR("MeshImporter: failed to import '{}': {}", path.string(), l_Importer.GetErrorString());
            return nullptr;
        }

        std::vector<Vertex> l_Vertices;
        std::vector<uint32_t> l_Indices;

        for (uint32_t l_MeshIndex = 0; l_MeshIndex < l_Scene->mNumMeshes; ++l_MeshIndex)
        {
            const aiMesh* a_Mesh = l_Scene->mMeshes[l_MeshIndex];

            const uint32_t l_BaseVertex = static_cast<uint32_t>(l_Vertices.size());

            for (uint32_t i = 0; i < a_Mesh->mNumVertices; ++i)
            {
                Vertex l_Vertex{};

                l_Vertex.Position = {
                    a_Mesh->mVertices[i].x,
                    a_Mesh->mVertices[i].y,
                    a_Mesh->mVertices[i].z
                };

                if (a_Mesh->HasNormals())
                {
                    l_Vertex.Normal = {
                        a_Mesh->mNormals[i].x,
                        a_Mesh->mNormals[i].y,
                        a_Mesh->mNormals[i].z
                    };
                }

                if (a_Mesh->mTextureCoords[0])
                {
                    l_Vertex.TexCoord = {
                        a_Mesh->mTextureCoords[0][i].x,
                        a_Mesh->mTextureCoords[0][i].y
                    };
                }

                l_Vertices.push_back(l_Vertex);
            }

            for (uint32_t i = 0; i < a_Mesh->mNumFaces; ++i)
            {
                const aiFace& l_Face = a_Mesh->mFaces[i];
                for (uint32_t j = 0; j < l_Face.mNumIndices; ++j)
                {
                    l_Indices.push_back(l_BaseVertex + l_Face.mIndices[j]);
                }
            }
        }

        if (l_Vertices.empty() || l_Indices.empty())
        {
            TR_CORE_ERROR("MeshImporter: '{}' produced no geometry", path.string());
            return nullptr;
        }

        TR_CORE_INFO("MeshImporter: imported '{}' — {} vertices, {} indices",
            path.filename().string(), l_Vertices.size(), l_Indices.size());

        auto l_Mesh = Mesh::Create(l_Vertices, l_Indices);

        const std::filesystem::path l_SourceDir = path.parent_path();
        for (uint32_t l_MeshIndex = 0; l_MeshIndex < l_Scene->mNumMeshes; ++l_MeshIndex)
        {
            const aiMesh* a_SubMesh = l_Scene->mMeshes[l_MeshIndex];
            if (a_SubMesh->mMaterialIndex >= l_Scene->mNumMaterials)
                continue;

            const aiMaterial* a_Material = l_Scene->mMaterials[a_SubMesh->mMaterialIndex];

            aiString l_DiffusePath;
            const aiTextureType l_Types[] = { aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR };
            bool l_Found = false;
            for (aiTextureType it_Type : l_Types)
            {
                if (a_Material->GetTexture(it_Type, 0, &l_DiffusePath) == AI_SUCCESS)
                {
                    l_Found = true;
                    break;
                }
            }
            if (!l_Found)
                continue;

            std::shared_ptr<Trinity::Texture> l_Texture;
            const char* l_PathStr = l_DiffusePath.C_Str();

            if (l_PathStr[0] == '*')
            {
                const int l_EmbeddedIndex = std::atoi(l_PathStr + 1);
                if (l_EmbeddedIndex >= 0 && static_cast<uint32_t>(l_EmbeddedIndex) < l_Scene->mNumTextures)
                {
                    const aiTexture* a_EmbTex = l_Scene->mTextures[l_EmbeddedIndex];
                    if (a_EmbTex->mHeight == 0)
                    {
                        l_Texture = Renderer::CreateTextureFromMemory(
                            reinterpret_cast<const uint8_t*>(a_EmbTex->pcData),
                            static_cast<size_t>(a_EmbTex->mWidth));
                    }
                    else
                    {
                        std::vector<uint8_t> l_Rgba(a_EmbTex->mWidth * a_EmbTex->mHeight * 4);
                        for (uint32_t i = 0; i < a_EmbTex->mWidth * a_EmbTex->mHeight; ++i)
                        {
                            l_Rgba[i * 4 + 0] = a_EmbTex->pcData[i].r;
                            l_Rgba[i * 4 + 1] = a_EmbTex->pcData[i].g;
                            l_Rgba[i * 4 + 2] = a_EmbTex->pcData[i].b;
                            l_Rgba[i * 4 + 3] = a_EmbTex->pcData[i].a;
                        }
                        l_Texture = Renderer::CreateTextureFromData(l_Rgba.data(), a_EmbTex->mWidth, a_EmbTex->mHeight);
                    }
                }
            }
            else
            {
                std::filesystem::path l_TexPath = l_PathStr;
                if (l_TexPath.is_relative())
                    l_TexPath = l_SourceDir / l_TexPath;

                l_Texture = Renderer::LoadTextureFromFile(l_TexPath.string());
            }

            if (l_Texture)
            {
                TR_CORE_INFO("MeshImporter: loaded albedo texture for '{}'", path.filename().string());
                l_Mesh->AlbedoTexture = l_Texture;
                break;
            }
            else
            {
                TR_CORE_WARN("MeshImporter: found diffuse slot '{}' but could not load it for '{}'", l_PathStr, path.filename().string());
            }
        }

        return l_Mesh;
    }
}
