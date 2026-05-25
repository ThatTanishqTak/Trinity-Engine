#include "Forge/ForgeLayer.h"

#include "Trinity/Events/Event.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Scene/Components/TransformComponent.h"
#include "Trinity/Scene/Components/MeshComponent.h"
#include "Trinity/Utilities/Log.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{

}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    TR_INFO("INITIALIZING FORGELAYER");

    m_Camera = Trinity::EditorCamera(60.0f, 1920.0f / 1080.0f, 0.1f, 1000.0f);
    m_SceneRenderer.Initialize(1920, 1080);

    TR_INFO("FORGELAYER INITIALIZED");
}

void ForgeLayer::OnShutdown()
{
    TR_INFO("SHUTTING DOWN FORGELAYER");

    Trinity::Renderer::WaitIdle();
    m_SceneRenderer.Shutdown();

    TR_INFO("FORGELAYER SHUTDOWN COMPLETE");
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    (void)deltaTime;
}

void ForgeLayer::OnRender()
{
    Trinity::SceneRenderData l_SceneData{};
    m_SceneRenderer.BeginScene(m_Camera, l_SceneData);

    auto& a_Registry = m_Scene.GetRegistry();
    auto a_View = a_Registry.view<Trinity::TransformComponent, Trinity::MeshComponent>();
    for (auto it_Entity : a_View)
    {
        auto& a_Transform = a_View.get<Trinity::TransformComponent>(it_Entity);
        auto& a_Mesh = a_View.get<Trinity::MeshComponent>(it_Entity);

        if (!a_Mesh.MeshData)
        {
            continue;
        }

        Trinity::MeshDrawCommand l_Command{};
        l_Command.MeshRef = a_Mesh.MeshData;

        const glm::mat4 l_ModelMatrix = a_Transform.GetWorldMatrix(m_Scene);
        std::memcpy(l_Command.Transform, glm::value_ptr(l_ModelMatrix), sizeof(l_Command.Transform));

        m_SceneRenderer.SubmitMesh(l_Command);
    }

    m_SceneRenderer.EndScene();
    m_SceneRenderer.Render();
}

void ForgeLayer::OnImGuiRender()
{

}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;
}