#include "ForgeLayer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"

#include "Trinity/ECS/SceneRenderer.h"
#include "Trinity/ECS/Components.h"

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{

}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    auto& a_Window = Trinity::Application::Get().GetWindow();
    m_EditorCamera.SetViewportSize(static_cast<float>(a_Window.GetWidth()), static_cast<float>(a_Window.GetHeight()));

    m_ActiveScene = std::make_unique<Trinity::Scene>();

    // Demo entities (so you can *see* the ECS working)
    {
        auto l_CubeA = m_ActiveScene->CreateEntity("Cube A");
        l_CubeA.AddComponent<Trinity::MeshRendererComponent>(Trinity::Geometry::PrimitiveType::Cube, glm::vec4(1, 0, 0, 1));
        l_CubeA.GetComponent<Trinity::TransformComponent>().Translation = { 0.0f, 0.0f, 0.0f };

        auto l_CubeB = m_ActiveScene->CreateEntity("Cube B");
        l_CubeB.AddComponent<Trinity::MeshRendererComponent>(Trinity::Geometry::PrimitiveType::Cube, glm::vec4(0, 1, 0, 1));
        l_CubeB.GetComponent<Trinity::TransformComponent>().Translation = { 2.0f, 0.0f, 0.0f };

        auto l_Quad = m_ActiveScene->CreateEntity("Quad");
        l_Quad.AddComponent<Trinity::MeshRendererComponent>(Trinity::Geometry::PrimitiveType::Quad, glm::vec4(0, 0.5f, 1, 1));
        l_Quad.GetComponent<Trinity::TransformComponent>().Translation = { -2.0f, 0.0f, 0.0f };
    }
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
    // Allow camera input while the scene viewport is active OR while we're in RMB look mode.
    m_EditorCamera.SetInputEnabled(m_CanControlCamera || m_IsLooking);
    m_EditorCamera.OnUpdate(deltaTime);
}

void ForgeLayer::OnRender()
{
    if (!m_ActiveScene)
    {
        return;
    }

    Trinity::SceneRenderer::Render(*m_ActiveScene, m_EditorCamera.GetViewMatrix(), m_EditorCamera.GetProjectionMatrix());
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

        auto& registry = m_ActiveScene->GetRegistry();
        entt::registry* regPtr = &registry;

        auto view = registry.view<Trinity::TagComponent>();
        for (auto e : view)
        {
            Trinity::Entity entity(e, regPtr);
            auto& tag = view.get<Trinity::TagComponent>(e);

            bool selected = (entity == m_SelectedEntity);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                (selected ? ImGuiTreeNodeFlags_Selected : 0);

            bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)e, flags, tag.Tag.c_str());

            if (ImGui::IsItemClicked())
                m_SelectedEntity = entity;

            bool deleteEntity = false;
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                    deleteEntity = true;
                ImGui::EndPopup();
            }

            if (opened)
                ImGui::TreePop();

            if (deleteEntity)
            {
                if (m_SelectedEntity == entity)
                    m_SelectedEntity = Trinity::Entity();

                m_ActiveScene->DestroyEntity(entity);
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
                auto& tag = m_SelectedEntity.GetComponent<Trinity::TagComponent>();

                char buffer[256]{};
                std::snprintf(buffer, sizeof(buffer), "%s", tag.Tag.c_str());

                if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
                    tag.Tag = buffer;
            }

            // Transform
            if (m_SelectedEntity.HasComponent<Trinity::TransformComponent>())
            {
                auto& tc = m_SelectedEntity.GetComponent<Trinity::TransformComponent>();

                ImGui::DragFloat3("Translation", &tc.Translation.x, 0.1f);
                ImGui::DragFloat3("Rotation", &tc.Rotation.x, 0.01f);
                ImGui::DragFloat3("Scale", &tc.Scale.x, 0.1f);
            }

            // MeshRenderer
            if (m_SelectedEntity.HasComponent<Trinity::MeshRendererComponent>())
            {
                auto& mr = m_SelectedEntity.GetComponent<Trinity::MeshRendererComponent>();

                ImGui::ColorEdit4("Color", &mr.Color.x);

                const char* items[] = { "Cube", "Quad" };
                int current = (mr.Primitive == Trinity::Geometry::PrimitiveType::Cube) ? 0 : 1;

                if (ImGui::Combo("Primitive", &current, items, IM_ARRAYSIZE(items)))
                    mr.Primitive = (current == 0) ? Trinity::Geometry::PrimitiveType::Cube : Trinity::Geometry::PrimitiveType::Quad;
            }
        }

        ImGui::End();
    }
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    Trinity::EventDispatcher l_Dispatcher(e);

    // If the window loses focus mid-look (alt-tab, click another window), stop looking and restore cursor.
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

    l_Dispatcher.Dispatch<Trinity::KeyPressedEvent>([this](Trinity::KeyPressedEvent& keyPressedEvent)
    {
        if (keyPressedEvent.GetKeyCode() == Trinity::Code::KeyCode::TR_KEY_ESCAPE)
        {
            // ESC exits RMB-look first.
            if (m_IsLooking)
            {
                auto& a_Window = Trinity::Application::Get().GetWindow();
                a_Window.SetCursorLocked(false);
                a_Window.SetCursorVisible(true);
                m_IsLooking = false;
            
                return true;
            }

            Trinity::Application::Close();

            return true;
        }
        return false;
    });

    // Only feed raw mouse deltas to the camera while in RMB look mode.
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
        // Avoid double-processing raw delta (handled above).
        if (e.GetEventType() != Trinity::EventType::MouseRawDelta)
        {
            m_EditorCamera.OnEvent(e);
        }
    }
}