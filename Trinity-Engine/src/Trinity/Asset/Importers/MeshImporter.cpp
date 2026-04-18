#include "Trinity/Asset/Importers/MeshImporter.h"

#include "Trinity/Utilities/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

        return Mesh::Create(l_Vertices, l_Indices);
    }
}
