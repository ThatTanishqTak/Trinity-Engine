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
        const aiScene* l_Scene = l_Importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes);

        if (!l_Scene || (l_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !l_Scene->mRootNode)
        {
            TR_CORE_ERROR("MeshImporter: failed to import '{}': {}", path.string(), l_Importer.GetErrorString());
            return nullptr;
        }

        std::vector<Vertex> l_Vertices;
        std::vector<uint32_t> l_Indices;

        for (uint32_t it_MeshIndex = 0; it_MeshIndex < l_Scene->mNumMeshes; ++it_MeshIndex)
        {
            const aiMesh* l_Mesh = l_Scene->mMeshes[it_MeshIndex];
            const uint32_t l_BaseVertex = static_cast<uint32_t>(l_Vertices.size());

            for (uint32_t i = 0; i < l_Mesh->mNumVertices; ++i)
            {
                Vertex l_Vertex{};

                l_Vertex.Position = { l_Mesh->mVertices[i].x, l_Mesh->mVertices[i].y, l_Mesh->mVertices[i].z };

                if (l_Mesh->HasNormals())
                {
                    l_Vertex.Normal = { l_Mesh->mNormals[i].x, l_Mesh->mNormals[i].y, l_Mesh->mNormals[i].z };
                }

                if (l_Mesh->mTextureCoords[0])
                {
                    l_Vertex.TextureCoordinate = { l_Mesh->mTextureCoords[0][i].x, l_Mesh->mTextureCoords[0][i].y };
                }

                l_Vertices.push_back(l_Vertex);
            }

            for (uint32_t i = 0; i < l_Mesh->mNumFaces; ++i)
            {
                const aiFace& l_Face = l_Mesh->mFaces[i];
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

        TR_CORE_INFO("MeshImporter: imported '{}' — {} vertices, {} indices", path.filename().string(), l_Vertices.size(), l_Indices.size());

        auto a_Mesh = Mesh::Create(l_Vertices, l_Indices);

        const std::filesystem::path l_SourceDir = path.parent_path();
        for (uint32_t it_MeshIndex = 0; it_MeshIndex < l_Scene->mNumMeshes; ++it_MeshIndex)
        {
            const aiMesh* l_SubMesh = l_Scene->mMeshes[it_MeshIndex];
            if (l_SubMesh->mMaterialIndex >= l_Scene->mNumMaterials)
            {
                continue;
            }

            const aiMaterial* l_Material = l_Scene->mMaterials[l_SubMesh->mMaterialIndex];
            aiString l_DiffusePath;

            const aiTextureType l_Types[] = { aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR };
            bool l_Found = false;

            for (aiTextureType it_Type : l_Types)
            {
                if (l_Material->GetTexture(it_Type, 0, &l_DiffusePath) == AI_SUCCESS)
                {
                    l_Found = true;

                    break;
                }
            }
            if (!l_Found)
            {
                continue;
            }

            std::shared_ptr<Trinity::Texture> l_Texture;
            const char* l_PathString = l_DiffusePath.C_Str();

            if (l_PathString[0] == '*')
            {
                const int l_EmbeddedIndex = std::atoi(l_PathString + 1);
                if (l_EmbeddedIndex >= 0 && static_cast<uint32_t>(l_EmbeddedIndex) < l_Scene->mNumTextures)
                {
                    const aiTexture* a_EmbededTexture = l_Scene->mTextures[l_EmbeddedIndex];
                    if (a_EmbededTexture->mHeight == 0)
                    {
                        l_Texture = Renderer::CreateTextureFromMemory(reinterpret_cast<const uint8_t*>(a_EmbededTexture->pcData), static_cast<size_t>(a_EmbededTexture->mWidth));
                    }
                    else
                    {
                        std::vector<uint8_t> l_RGBA(a_EmbededTexture->mWidth * a_EmbededTexture->mHeight * 4);
                        for (uint32_t i = 0; i < a_EmbededTexture->mWidth * a_EmbededTexture->mHeight; ++i)
                        {
                            l_RGBA[i * 4 + 0] = a_EmbededTexture->pcData[i].r;
                            l_RGBA[i * 4 + 1] = a_EmbededTexture->pcData[i].g;
                            l_RGBA[i * 4 + 2] = a_EmbededTexture->pcData[i].b;
                            l_RGBA[i * 4 + 3] = a_EmbededTexture->pcData[i].a;
                        }

                        l_Texture = Renderer::CreateTextureFromData(l_RGBA.data(), a_EmbededTexture->mWidth, a_EmbededTexture->mHeight);
                    }
                }
            }
            else
            {
                std::filesystem::path l_TexturePath = l_PathString;
                if (l_TexturePath.is_relative())
                {
                    l_TexturePath = l_SourceDir / l_TexturePath;
                }

                l_Texture = Renderer::LoadTextureFromFile(l_TexturePath.string());
            }

            if (l_Texture)
            {
                TR_CORE_INFO("MeshImporter: loaded albedo texture for '{}'", path.filename().string());
                a_Mesh->AlbedoTexture = l_Texture;

                break;
            }
            else
            {
                TR_CORE_WARN("MeshImporter: found diffuse slot '{}' but could not load it for '{}'", l_PathString, path.filename().string());
            }
        }

        return a_Mesh;
    }
}