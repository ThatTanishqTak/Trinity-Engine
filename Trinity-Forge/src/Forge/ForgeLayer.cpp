#include "ForgeLayer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"

#include "Trinity/ECS/SceneRenderer.h"
#include "Trinity/ECS/Components.h"
#include "Trinity/ECS/SceneSerializer.h"
#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Assets/BuiltInAssets.h"

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{

}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::LoadScene(std::unique_ptr<Trinity::Scene> scene)
{
    m_SelectedEntity = Trinity::Entity{};
    m_ActiveScene = std::move(scene);
}

void ForgeLayer::OnInitialize()
{
    auto& a_Window = Trinity::Application::Get().GetWindow();
    m_EditorCamera.SetViewportSize(static_cast<float>(a_Window.GetWidth()), static_cast<float>(a_Window.GetHeight()));

    Trinity::BuiltIn::RegisterAll();

    auto l_Scene = std::make_unique<Trinity::Scene>();

    // Demo entities
    {
        auto l_CubeA = l_Scene->CreateEntity("Cube A");
        l_CubeA.AddComponent<Trinity::MeshRendererComponent>(Trinity::BuiltIn::Cube(), glm::vec4(1, 0, 0, 1));
        l_CubeA.GetComponent<Trinity::TransformComponent>().Translation = { 0.0f, 0.0f, 0.0f };

        auto l_CubeB = l_Scene->CreateEntity("Cube B");
        l_CubeB.AddComponent<Trinity::MeshRendererComponent>(Trinity::BuiltIn::Cube(), glm::vec4(0, 1, 0, 1));
        l_CubeB.GetComponent<Trinity::TransformComponent>().Translation = { 2.0f, 0.0f, 0.0f };

        auto l_Quad = l_Scene->CreateEntity("Quad");
        l_Quad.AddComponent<Trinity::MeshRendererComponent>(Trinity::BuiltIn::Quad(), glm::vec4(0, 0.5f, 1, 1));
        l_Quad.GetComponent<Trinity::TransformComponent>().Translation = { -2.0f, 0.0f, 0.0f };
    }

    LoadScene(std::move(l_Scene));
}

void ForgeLayer::OnShutdown()
{
    // Safety: never leave the cursor clipped/hidden if the layer shuts down mid-look.
    if (m_IsLooking)
    {
        auto& a_Window = Trinity::Application::Get().GetWindow();
        a_Window.SetCursorLocked(false);
        a_Window.SetCursorVisible(true);
        m_IsLooking = false;
    }
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    m_EditorCamera.SetInputEnabled(m_CanControlCamera || m_IsLooking);
    m_EditorCamera.OnUpdate(deltaTime);
}

void ForgeLayer::OnRender()
{
    if (!m_ActiveScene)
    {
        return;
    }

    Trinity::SceneRenderer::Render(*m_ActiveScene, m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjectionMatrix(), m_EditorCamera.GetPosition(), m_EditorCamera.GetNearClip(), m_EditorCamera.GetFarClip());
}

void ForgeLayer::OnImGuiRender()
{
    ImGuiWindowFlags l_WindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* l_MainViewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(l_MainViewport->Pos);
    ImGui::SetNextWindowSize(l_MainViewport->Size);
    ImGui::SetNextWindowViewport(l_MainViewport->ID);

    l_WindowFlags |= ImGuiWindowFlags_NoTitleBar;
    l_WindowFlags |= ImGuiWindowFlags_NoCollapse;
    l_WindowFlags |= ImGuiWindowFlags_NoResize;
    l_WindowFlags |= ImGuiWindowFlags_NoMove;
    l_WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    l_WindowFlags |= ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("ForgeDockspace", nullptr, l_WindowFlags);
    ImGui::PopStyleVar(2);

    const ImGuiID l_DockspaceID = ImGui::GetID("ForgeDockspaceID");
    ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::Begin("Scene");

    const bool l_IsSceneViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const bool l_IsSceneViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    m_CanControlCamera = l_IsSceneViewportFocused && l_IsSceneViewportHovered;

    auto& a_Window = Trinity::Application::Get().GetWindow();
    if (m_CanControlCamera && !m_IsLooking && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        m_IsLooking = true;
        a_Window.SetCursorVisible(false);
        a_Window.SetCursorLocked(true);
    }

    if (m_IsLooking && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        m_IsLooking = false;
        a_Window.SetCursorLocked(false);
        a_Window.SetCursorVisible(true);
    }

    const ImVec2 l_ViewportPanelSize = ImGui::GetContentRegionAvail();
    const uint32_t l_SceneViewportWidth = static_cast<uint32_t>(l_ViewportPanelSize.x > 0.0f ? l_ViewportPanelSize.x : 0.0f);
    const uint32_t l_SceneViewportHeight = static_cast<uint32_t>(l_ViewportPanelSize.y > 0.0f ? l_ViewportPanelSize.y : 0.0f);

    Trinity::RenderCommand::SetSceneViewportSize(l_SceneViewportWidth, l_SceneViewportHeight);

    const ImVec2 l_SceneViewportSize = ImVec2(static_cast<float>(l_SceneViewportWidth), static_cast<float>(l_SceneViewportHeight));
    if (l_SceneViewportSize.x != m_SceneViewportSize.x || l_SceneViewportSize.y != m_SceneViewportSize.y)
    {
        m_SceneViewportSize = l_SceneViewportSize;

        if (l_SceneViewportWidth > 0 && l_SceneViewportHeight > 0)
        {
            m_EditorCamera.SetViewportSize(static_cast<float>(l_SceneViewportWidth), static_cast<float>(l_SceneViewportHeight));
        }
    }

    ImTextureID l_SceneTexture = reinterpret_cast<ImTextureID>(Trinity::RenderCommand::GetSceneViewportHandle());
    if (l_SceneTexture != (ImTextureID)nullptr && l_ViewportPanelSize.x > 0.0f && l_ViewportPanelSize.y > 0.0f)
    {
        ImGui::Image(l_SceneTexture, l_ViewportPanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    }

    ImGui::End(); // Scene
    ImGui::End(); // Dockspace

    if (m_ActiveScene)
    {
        ImGui::Begin("Scene Hierarchy");

        // Create entity button
        if (ImGui::Button("Create Entity"))
        {
            m_SelectedEntity = m_ActiveScene->CreateEntity("Empty Entity");
        }

        ImGui::Separator();

        auto& a_Registry = m_ActiveScene->GetRegistry();
        entt::registry* l_RegistryPointer = &a_Registry;

        auto a_View = a_Registry.view<Trinity::TagComponent>();
        for (auto it_Entity : a_View)
        {
            Trinity::Entity l_Entity(it_Entity, l_RegistryPointer);
            auto& a_TagComponent = a_View.get<Trinity::TagComponent>(it_Entity);

            bool l_Selected = (l_Entity == m_SelectedEntity);

            ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | (l_Selected ? ImGuiTreeNodeFlags_Selected : 0);

            bool l_Opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)it_Entity, l_Flags, a_TagComponent.Tag.c_str());

            if (ImGui::IsItemClicked())
            {
                m_SelectedEntity = l_Entity;
            }

            bool l_DeleteEntity = false;
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    l_DeleteEntity = true;
                }
                ImGui::EndPopup();
            }

            if (l_Opened)
            {
                ImGui::TreePop();
            }

            if (l_DeleteEntity)
            {
                if (m_SelectedEntity == l_Entity)
                {
                    m_SelectedEntity = Trinity::Entity();
                }

                m_ActiveScene->DestroyEntity(l_Entity);
                break; // registry modified, break out
            }
        }

        // Click blank space to clear selection
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            m_SelectedEntity = Trinity::Entity();
        }

        ImGui::End();

        ImGui::Begin("Properties");

        if (m_SelectedEntity)
        {
            // Tag
            if (m_SelectedEntity.HasComponent<Trinity::TagComponent>())
            {
                auto& a_TagComponent = m_SelectedEntity.GetComponent<Trinity::TagComponent>();

                char l_Buffer[256]{};
                std::snprintf(l_Buffer, sizeof(l_Buffer), "%s", a_TagComponent.Tag.c_str());

                if (ImGui::InputText("Tag", l_Buffer, sizeof(l_Buffer)))
                {
                    a_TagComponent.Tag = l_Buffer;
                }
            }

            // Transform
            if (m_SelectedEntity.HasComponent<Trinity::TransformComponent>())
            {
                auto& a_TransformComponent = m_SelectedEntity.GetComponent<Trinity::TransformComponent>();

                ImGui::DragFloat3("Translation", &a_TransformComponent.Translation.x, 0.1f);
                ImGui::DragFloat3("Rotation", &a_TransformComponent.Rotation.x, 0.01f);
                ImGui::DragFloat3("Scale", &a_TransformComponent.Scale.x, 0.1f);
            }

            // MeshRenderer
            if (m_SelectedEntity.HasComponent<Trinity::MeshRendererComponent>())
            {
                auto& a_MeshRendererComponent = m_SelectedEntity.GetComponent<Trinity::MeshRendererComponent>();

                ImGui::ColorEdit4("Color", &a_MeshRendererComponent.Color.x);

                char l_UUIDBuffer[32];
                snprintf(l_UUIDBuffer, sizeof(l_UUIDBuffer), "%llu", static_cast<unsigned long long>(a_MeshRendererComponent.Mesh.GetUUID()));
                ImGui::LabelText("Mesh UUID", "%s", l_UUIDBuffer);
            }

            // Add Component Button
            if (ImGui::Button("Add Component", ImVec2(10.0f, 10.0f)))
            {

            }
        }

        ImGui::End();
    }
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    Trinity::EventDispatcher l_Dispatcher(e);

    // If the window loses focus mid-look (alt-tab, click another window), stop looking and restore cursor
    l_Dispatcher.Dispatch<Trinity::WindowLostFocusEvent>([this](Trinity::WindowLostFocusEvent&)
        {
            if (m_IsLooking)
            {
                auto& a_Window = Trinity::Application::Get().GetWindow();
                a_Window.SetCursorLocked(false);
                a_Window.SetCursorVisible(true);
                m_IsLooking = false;
            }

            return false;
        });

    // Only feed raw mouse deltas to the camera while in RMB look mode
    l_Dispatcher.Dispatch<Trinity::MouseRawDeltaEvent>([this](Trinity::MouseRawDeltaEvent& rawDeltaEvent)
        {
            if (!m_IsLooking)
            {
                return false;
            }

            m_EditorCamera.OnEvent(rawDeltaEvent);

            return true;
        });

    if (!e.Handled && (m_CanControlCamera || m_IsLooking))
    {
        // Avoid double-processing raw delta
        if (e.GetEventType() != Trinity::EventType::MouseRawDelta)
        {
            m_EditorCamera.OnEvent(e);
        }
    }
}