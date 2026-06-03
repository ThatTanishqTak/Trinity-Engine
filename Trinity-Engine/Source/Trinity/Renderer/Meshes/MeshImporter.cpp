#include <Trinity/Renderer/Meshes/MeshImporter.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <format>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    struct MeshImporter::Implementation
    {
        Assimp::Importer Importer;
    };

    MeshImporter::MeshImporter() : m_Implementation(std::make_unique<Implementation>())
    {

    }

    MeshImporter::~MeshImporter() = default;

    std::optional<MeshData> MeshImporter::Import(const std::filesystem::path& path)
    {
        const std::string l_PathString = path.string();

        const unsigned int l_Flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_PreTransformVertices | aiProcess_FlipUVs;

        const aiScene* l_Scene = m_Implementation->Importer.ReadFile(l_PathString, l_Flags);
        if (l_Scene == nullptr || (l_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || l_Scene->mRootNode == nullptr)
        {
            TR_CORE_ERROR("MeshImporter: failed to load '{}' ({})", l_PathString, m_Implementation->Importer.GetErrorString());

            return std::nullopt;
        }

        MeshData l_Data;
        l_Data.Diagnostics.SourcePath = l_PathString;
        l_Data.Diagnostics.SourceFormat = path.extension().string();

        l_Data.MaterialSlots.reserve(l_Scene->mNumMaterials);
        for (unsigned int i = 0; i < l_Scene->mNumMaterials; ++i)
        {
            const aiMaterial* l_Material = l_Scene->mMaterials[i];

            MaterialSlot l_Slot;
            aiString l_Name;
            if (l_Material->Get(AI_MATKEY_NAME, l_Name) == AI_SUCCESS)
            {
                l_Slot.Name = l_Name.C_Str();
            }

            aiColor4D l_BaseColor;
            if (l_Material->Get(AI_MATKEY_BASE_COLOR, l_BaseColor) == AI_SUCCESS || l_Material->Get(AI_MATKEY_COLOR_DIFFUSE, l_BaseColor) == AI_SUCCESS)
            {
                l_Slot.BaseColorFactor = glm::vec4(l_BaseColor.r, l_BaseColor.g, l_BaseColor.b, l_BaseColor.a);
            }

            aiString l_TexturePath;
            if (l_Material->GetTexture(aiTextureType_BASE_COLOR, 0, &l_TexturePath) == AI_SUCCESS || l_Material->GetTexture(aiTextureType_DIFFUSE, 0, &l_TexturePath) == AI_SUCCESS)
            {
                l_Slot.BaseColorTexturePath = l_TexturePath.C_Str();
            }

            l_Data.MaterialSlots.push_back(l_Slot);
        }

        for (unsigned int i = 0; i < l_Scene->mNumMeshes; ++i)
        {
            const aiMesh* l_Mesh = l_Scene->mMeshes[i];

            Submesh l_Submesh;
            l_Submesh.BaseVertex = static_cast<uint32_t>(l_Data.Vertices.size());
            l_Submesh.FirstIndex = static_cast<uint32_t>(l_Data.Indices.size());
            l_Submesh.MaterialIndex = l_Mesh->mMaterialIndex;
            l_Submesh.Name = l_Mesh->mName.C_Str();

            const bool l_HasUV = l_Mesh->HasTextureCoords(0);
            const bool l_HasNormals = l_Mesh->HasNormals();
            const bool l_HasTangents = l_Mesh->HasTangentsAndBitangents();
            if (!l_HasUV)
            {
                l_Data.Diagnostics.Warnings.push_back(std::format("submesh '{}' has no UVs", l_Submesh.Name));
            }

            l_Data.Vertices.reserve(l_Data.Vertices.size() + l_Mesh->mNumVertices);
            for (unsigned int l_Vi = 0; l_Vi < l_Mesh->mNumVertices; ++l_Vi)
            {
                MeshVertex l_Vertex{};
                l_Vertex.Position = { l_Mesh->mVertices[l_Vi].x, l_Mesh->mVertices[l_Vi].y, l_Mesh->mVertices[l_Vi].z };
                if (l_HasNormals)
                {
                    l_Vertex.Normal = { l_Mesh->mNormals[l_Vi].x, l_Mesh->mNormals[l_Vi].y, l_Mesh->mNormals[l_Vi].z };
                }

                if (l_HasTangents)
                {
                    l_Vertex.Tangent = { l_Mesh->mTangents[l_Vi].x, l_Mesh->mTangents[l_Vi].y, l_Mesh->mTangents[l_Vi].z };
                }
                
                if (l_HasUV)
                {
                    l_Vertex.UV = { l_Mesh->mTextureCoords[0][l_Vi].x, l_Mesh->mTextureCoords[0][l_Vi].y };
                }

                l_Data.Vertices.push_back(l_Vertex);
            }

            uint32_t l_IndexCount = 0;
            for (unsigned int l_Fi = 0; l_Fi < l_Mesh->mNumFaces; ++l_Fi)
            {
                const aiFace& l_Face = l_Mesh->mFaces[l_Fi];
                if (l_Face.mNumIndices != 3)
                {
                    continue;
                }

                l_Data.Indices.push_back(l_Face.mIndices[0]);
                l_Data.Indices.push_back(l_Face.mIndices[1]);
                l_Data.Indices.push_back(l_Face.mIndices[2]);
                l_IndexCount += 3;
            }

            l_Submesh.IndexCount = l_IndexCount;
            l_Data.Submeshes.push_back(l_Submesh);
        }

        if (l_Data.Vertices.empty() || l_Data.Indices.empty())
        {
            TR_CORE_ERROR("MeshImporter: '{}' produced no renderable geometry", l_PathString);

            return std::nullopt;
        }

        TR_CORE_INFO("MeshImporter: loaded '{}' ({} submeshes, {} vertices, {} indices)", l_PathString, l_Data.Submeshes.size(), l_Data.Vertices.size(), l_Data.Indices.size());
        for (const std::string& l_Warning : l_Data.Diagnostics.Warnings)
        {
            TR_CORE_WARN("MeshImporter: {}", l_Warning);
        }

        return l_Data;
    }
}