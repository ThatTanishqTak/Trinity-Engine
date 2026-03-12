#include "Trinity/Assets/BuiltInAssets.h"

#include "Trinity/Assets/AssetManager.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"
#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    namespace BuiltIn
    {
        void RegisterAll()
        {
            VulkanRenderer& l_Renderer = static_cast<VulkanRenderer&>(RenderCommand::GetRenderer());
            VulkanAllocator& l_Allocator = l_Renderer.GetVMAAllocator();
            VulkanUploadContext& l_UploadContext = l_Renderer.GetUploadContext();

            AssetManager& l_Manager = AssetManager::Get();

            const std::pair<AssetUUID, Geometry::PrimitiveType> l_Primitives[] =
            {
                { TriangleMeshUUID, Geometry::PrimitiveType::Triangle },
                { QuadMeshUUID, Geometry::PrimitiveType::Quad },
                { CubeMeshUUID, Geometry::PrimitiveType::Cube },
            };

            const char* l_Names[] = { "BuiltIn/Triangle", "BuiltIn/Quad", "BuiltIn/Cube" };

            for (int l_I = 0; l_I < 3; l_I++)
            {
                const auto& [l_UUID, l_Type] = l_Primitives[l_I];
                const Geometry::MeshData& l_Data = Geometry::GetPrimitive(l_Type);
                auto l_Mesh = MeshAsset::CreateFromMeshData(l_Data, l_Allocator, l_UploadContext, l_Names[l_I]);
                l_Manager.Register<MeshAsset>(std::move(l_Mesh), l_UUID);
            }

            TR_CORE_INFO("BuiltInAssets: registered Triangle, Quad, Cube");
        }

        AssetHandle<MeshAsset> Triangle()
        {
            return AssetHandle<MeshAsset>(TriangleMeshUUID);
        }

        AssetHandle<MeshAsset> Quad()
        {
            return AssetHandle<MeshAsset>(QuadMeshUUID);
        }

        AssetHandle<MeshAsset> Cube()
        {
            return AssetHandle<MeshAsset>(CubeMeshUUID);
        }
    }
}