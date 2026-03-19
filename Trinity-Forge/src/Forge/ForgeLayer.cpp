#include "ForgeLayer.h"

#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Platform/Window.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Events/ApplicationEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"
#include "Trinity/Input/Input.h"

#include "Trinity/ECS/SceneRenderer.h"
#include "Trinity/ECS/Components.h"
#include "Trinity/ECS/SceneSerializer.h"
#include "Trinity/Utilities/FileManagement.h"
#include "Trinity/Assets/BuiltInAssets.h"
#include "Trinity/Assets/AssetRegistry.h"

#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <entt/entt.hpp>

static void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 120.0f)
{
    ImGui::PushID(label);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label);
    ImGui::NextColumn();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    const float l_LineHeight = ImGui::GetFrameHeight();
    const ImVec2 l_ButtonSize = { l_LineHeight + 3.0f, l_LineHeight };
    const float l_ItemWidth = (ImGui::GetContentRegionAvail().x - l_ButtonSize.x * 3.0f) / 3.0f;

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.10f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.10f, 0.15f, 1.0f));
    if (ImGui::Button("X", l_ButtonSize))
    { 
        values.x = resetValue;
    }

    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(l_ItemWidth);
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.3f");
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.70f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.80f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.70f, 0.20f, 1.0f));
    if (ImGui::Button("Y", l_ButtonSize))
    {
        values.y = resetValue;
    }

    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(l_ItemWidth);
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.3f");
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.25f, 0.80f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.35f, 0.90f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.25f, 0.80f, 1.0f));
    if (ImGui::Button("Z", l_ButtonSize))
    {
        values.z = resetValue;
    }

    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(l_ItemWidth);
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.3f");

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

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

    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    Trinity::BuiltIn::RegisterAll();
    Trinity::AssetRegistry::Get().Scan("Assets");

    auto l_Scene = std::make_unique<Trinity::Scene>();

    {
        auto l_Sun = l_Scene->CreateEntity("Directional Light");
        auto& l_Light = l_Sun.AddComponent<Trinity::LightComponent>();
        l_Light.Type = Trinity::LightType::Directional;
        l_Light.Color = glm::vec3(1.0f);
        l_Light.Intensity = 5.0f;
        l_Light.CastShadows = true;
        l_Sun.GetComponent<Trinity::TransformComponent>().Rotation = glm::vec3(glm::radians(-45.0f), glm::radians(20.0f), 0.0f);

        auto l_CubeA = l_Scene->CreateEntity("Cube A");
        l_CubeA.AddComponent<Trinity::MeshRendererComponent>(Trinity::BuiltIn::Cube(), glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
        l_CubeA.GetComponent<Trinity::TransformComponent>().Translation = { -2.0f, 0.0f, 0.0f };

        auto l_CubeB = l_Scene->CreateEntity("Cube B");
        l_CubeB.AddComponent<Trinity::MeshRendererComponent>(Trinity::BuiltIn::Cube(), glm::vec4(0.2f, 0.7f, 0.3f, 1.0f));
        l_CubeB.GetComponent<Trinity::TransformComponent>().Translation = { 0.0f, 0.0f, 0.0f };

        auto l_Quad = l_Scene->CreateEntity("Ground Quad");
        l_Quad.AddComponent<Trinity::MeshRendererComponent>(Trinity::BuiltIn::Quad(), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        auto& a_QuadTransform = l_Quad.GetComponent<Trinity::TransformComponent>();
        a_QuadTransform.Translation = { 0.0f, -1.0f, 0.0f };
        a_QuadTransform.Rotation = glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f);
        a_QuadTransform.Scale = glm::vec3(10.0f, 10.0f, 1.0f);
    }

    LoadScene(std::move(l_Scene));
}

void ForgeLayer::OnShutdown()
{
    if (m_IsLooking)
    {
        auto& a_Window = Trinity::Application::Get().GetWindow();
        a_Window.SetCursorLocked(false);
        a_Window.SetCursorVisible(true);
        m_IsLooking = false;
    }

    m_SelectedEntity = Trinity::Entity{};
    m_ActiveScene.reset();
    m_CurrentScenePath.clear();
    m_SceneSnapshot.clear();
    m_SceneViewportSize = ImVec2(0.0f, 0.0f);
    m_CanControlCamera = false;
    m_EditorState = EditorState::Edit;
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    const bool l_InputAllowed = (m_CanControlCamera || m_IsLooking);
    m_EditorCamera.SetInputEnabled(l_InputAllowed);
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
    ImGuizmo::BeginFrame();

    // Dockspace window
    {
        const ImGuiWindowFlags l_WindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(l_Viewport->Pos);
        ImGui::SetNextWindowSize(l_Viewport->Size);
        ImGui::SetNextWindowViewport(l_Viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("ForgeDockspace", nullptr, l_WindowFlags);
        ImGui::PopStyleVar(3);

        DrawMenuBar();

        const ImGuiID l_DockspaceID = ImGui::GetID("ForgeDockspaceID");
        ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }

    DrawToolbar();
    DrawSceneViewport();
    DrawSceneHierarchy();
    DrawInspector();
    m_ContentBrowser.OnImGuiRender();
}

void ForgeLayer::DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
            {
                NewScene();
            }

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
            {
                const auto l_Path = Trinity::Utilities::FileManagement::OpenFileDialog({ { "Trinity Scene", "*.trinity" } });
                if (l_Path.has_value())
                {
                    OpenScene(l_Path.value());
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                SaveScene();
            }

            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
            {
                SaveSceneAs();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                Trinity::Application::Close();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset Camera"))
            {
                m_EditorCamera.SetPosition(glm::vec3(0.0f, 0.0f, 8.0f));
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void ForgeLayer::DrawToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // --- Gizmo mode ---
    const float l_BtnW = 72.0f;
    const float l_BtnH = 0.0f;

    auto l_PushActive = [](bool active)
    {
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.36f, 0.69f, 1.00f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.49f, 0.88f, 1.0f));
        }
    };

    auto a_PopActive = [](bool active)
    {
        if (active)
        {
            ImGui::PopStyleColor(3);
        }
    };

    const bool l_IsTranslate = (m_GizmoOperation == (int)ImGuizmo::TRANSLATE);
    const bool l_IsRotate = (m_GizmoOperation == (int)ImGuizmo::ROTATE);
    const bool l_IsScale = (m_GizmoOperation == (int)ImGuizmo::SCALE);

    l_PushActive(l_IsTranslate);
    if (ImGui::Button("Translate", ImVec2(l_BtnW, l_BtnH)))
    {
        m_GizmoOperation = (int)ImGuizmo::TRANSLATE;
    }

    a_PopActive(l_IsTranslate);
    ImGui::SameLine();

    l_PushActive(l_IsRotate);
    if (ImGui::Button("Rotate", ImVec2(l_BtnW, l_BtnH)))
    {
        m_GizmoOperation = (int)ImGuizmo::ROTATE;
    }

    a_PopActive(l_IsRotate);
    ImGui::SameLine();

    l_PushActive(l_IsScale);
    if (ImGui::Button("Scale", ImVec2(l_BtnW, l_BtnH)))
    {
        m_GizmoOperation = (int)ImGuizmo::SCALE;
    }

    a_PopActive(l_IsScale);
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    l_PushActive(m_GizmoLocal);
    if (ImGui::Button(m_GizmoLocal ? "Local" : "World", ImVec2(l_BtnW * 0.8f, l_BtnH)))
    {
        m_GizmoLocal = !m_GizmoLocal;
    }

    a_PopActive(m_GizmoLocal);
    ImGui::SameLine();

    ImGui::Checkbox("Snap", &m_GizmoSnap);

    const float l_PlayBtnW = 64.0f;
    const float l_WindowW = ImGui::GetWindowWidth();

    float l_CenterX = 0.0f;
    if (m_EditorState == EditorState::Edit)
    {
        l_CenterX = (l_WindowW - l_PlayBtnW) * 0.5f;
    }
    else
    {
        l_CenterX = (l_WindowW - l_PlayBtnW * 2.0f - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    }

    if (l_CenterX > ImGui::GetCursorPosX())
    {
        ImGui::SetCursorPosX(l_CenterX);
    }

    if (m_EditorState == EditorState::Edit)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.55f, 0.10f, 1.0f));
        if (ImGui::Button("  Play  ", ImVec2(l_PlayBtnW, l_BtnH)))
        {
            OnPlay();
        }

        ImGui::PopStyleColor(3);
    }
    else
    {
        if (m_EditorState == EditorState::Paused)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.55f, 0.10f, 1.0f));
            if (ImGui::Button("Resume", ImVec2(l_PlayBtnW, l_BtnH)))
            {
                OnPause();
            }

            ImGui::PopStyleColor(3);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.70f, 0.10f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.80f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.60f, 0.05f, 1.0f));
            if (ImGui::Button(" Pause ", ImVec2(l_PlayBtnW, l_BtnH)))
            {
                OnPause();
            }

            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.10f, 0.10f, 1.0f));
        if (ImGui::Button("  Stop  ", ImVec2(l_PlayBtnW, l_BtnH)))
        {
            OnStop();
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ForgeLayer::DrawSceneViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Scene");
    ImGui::PopStyleVar();

    const bool l_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const bool l_Hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    m_CanControlCamera = l_Focused && l_Hovered;

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
    const uint32_t l_ViewportW = static_cast<uint32_t>(l_ViewportPanelSize.x > 0.0f ? l_ViewportPanelSize.x : 0.0f);
    const uint32_t l_ViewportH = static_cast<uint32_t>(l_ViewportPanelSize.y > 0.0f ? l_ViewportPanelSize.y : 0.0f);

    Trinity::RenderCommand::SetSceneViewportSize(l_ViewportW, l_ViewportH);

    const ImVec2 l_NewSize = ImVec2(static_cast<float>(l_ViewportW), static_cast<float>(l_ViewportH));
    if (l_NewSize.x != m_SceneViewportSize.x || l_NewSize.y != m_SceneViewportSize.y)
    {
        m_SceneViewportSize = l_NewSize;
        if (l_ViewportW > 0 && l_ViewportH > 0)
        {
            m_EditorCamera.SetViewportSize(static_cast<float>(l_ViewportW), static_cast<float>(l_ViewportH));
        }
    }

    const ImVec2 l_CursorScreen = ImGui::GetCursorScreenPos();
    m_SceneViewportBounds[0] = l_CursorScreen;
    m_SceneViewportBounds[1] = ImVec2(l_CursorScreen.x + l_ViewportPanelSize.x, l_CursorScreen.y + l_ViewportPanelSize.y);

    const ImTextureID l_SceneTexture = reinterpret_cast<ImTextureID>(Trinity::RenderCommand::GetSceneViewportHandle());
    if (l_SceneTexture != (ImTextureID) nullptr && l_ViewportPanelSize.x > 0.0f && l_ViewportPanelSize.y > 0.0f)
    {
        ImGui::Image(l_SceneTexture, l_ViewportPanelSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    }

    // --- Gizmo overlay ---
    if (m_ActiveScene && m_SelectedEntity && m_EditorState == EditorState::Edit && m_GizmoOperation != -1)
    {
        if (m_SelectedEntity.HasComponent<Trinity::TransformComponent>())
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

            ImGuizmo::SetRect(m_SceneViewportBounds[0].x, m_SceneViewportBounds[0].y, m_SceneViewportBounds[1].x - m_SceneViewportBounds[0].x, m_SceneViewportBounds[1].y - m_SceneViewportBounds[0].y);

            const glm::mat4 l_View = m_EditorCamera.GetViewMatrix();
            const glm::mat4 l_Projection = m_EditorCamera.GetProjectionMatrix();

            auto& a_Transform = m_SelectedEntity.GetComponent<Trinity::TransformComponent>();
            glm::mat4 l_TransformMatrix = a_Transform.GetTransform();

            float l_Snap[3] = { m_TranslationSnap, m_TranslationSnap, m_TranslationSnap };
            if (m_GizmoOperation == (int)ImGuizmo::ROTATE)
            {
                l_Snap[0] = l_Snap[1] = l_Snap[2] = m_RotationSnap;
            }
            else if (m_GizmoOperation == (int)ImGuizmo::SCALE)
            {
                l_Snap[0] = l_Snap[1] = l_Snap[2] = m_ScaleSnap;
            }

            ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), (ImGuizmo::OPERATION)m_GizmoOperation, m_GizmoLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD, glm::value_ptr(l_TransformMatrix), nullptr, m_GizmoSnap ? l_Snap : nullptr);

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 l_Translation{};
                glm::vec3 l_RotationDegrees{};
                glm::vec3 l_Scale{};

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_TransformMatrix), glm::value_ptr(l_Translation), glm::value_ptr(l_RotationDegrees), glm::value_ptr(l_Scale));

                a_Transform.Translation = l_Translation;
                a_Transform.Rotation = glm::radians(l_RotationDegrees);
                a_Transform.Scale = l_Scale;
            }
        }
    }

    ImGui::End();
}

void ForgeLayer::DrawSceneHierarchy()
{
    ImGui::Begin("Scene Hierarchy");

    if (m_EditorState != EditorState::Edit)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Create Entity"))
    {
        m_SelectedEntity = m_ActiveScene->CreateEntity("Empty Entity");
    }

    if (m_EditorState != EditorState::Edit)
    {
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    if (m_ActiveScene)
    {
        auto& a_Registry = m_ActiveScene->GetRegistry();
        auto a_View = a_Registry.view<Trinity::TagComponent>();

        for (auto it_Entity : a_View)
        {
            Trinity::Entity l_Entity(it_Entity, &a_Registry);
            DrawEntityNode(l_Entity);
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        {
            m_SelectedEntity = Trinity::Entity{};
        }

        if (ImGui::BeginPopupContextWindow("##HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            if (m_EditorState == EditorState::Edit)
            {
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    m_SelectedEntity = m_ActiveScene->CreateEntity("Empty Entity");
                }
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void ForgeLayer::DrawEntityNode(Trinity::Entity entity)
{
    auto& a_Tag = entity.GetComponent<Trinity::TagComponent>();

    const ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ((entity == m_SelectedEntity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
    const bool l_Open = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)(entt::entity)entity.GetHandle(), l_Flags, "%s", a_Tag.Tag.c_str());

    if (ImGui::IsItemClicked() && m_EditorState == EditorState::Edit)
    {
        m_SelectedEntity = entity;
    }

    bool l_DeleteEntity = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (m_EditorState == EditorState::Edit && ImGui::MenuItem("Delete Entity"))
        {
            l_DeleteEntity = true;
        }
        ImGui::EndPopup();
    }

    if (l_Open)
    {
        ImGui::TreePop();
    }

    if (l_DeleteEntity)
    {
        if (m_SelectedEntity == entity)
        {
            m_SelectedEntity = Trinity::Entity{};
        }
        m_ActiveScene->DestroyEntity(entity);
    }
}

void ForgeLayer::DrawInspector()
{
    ImGui::Begin("Properties");

    if (m_SelectedEntity)
    {
        DrawComponents(m_SelectedEntity);

        ImGui::Separator();

        if (m_EditorState == EditorState::Edit)
        {
            const float l_ButtonWidth = ImGui::GetContentRegionAvail().x;
            if (ImGui::Button("Add Component", ImVec2(l_ButtonWidth, 0.0f)))
            {
                ImGui::OpenPopup("##AddComponent");
            }

            if (ImGui::BeginPopup("##AddComponent"))
            {
                DrawAddComponentMenu(m_SelectedEntity);
                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

void ForgeLayer::DrawComponents(Trinity::Entity entity)
{
    // Tag
    if (entity.HasComponent<Trinity::TagComponent>())
    {
        auto& a_Tag = entity.GetComponent<Trinity::TagComponent>();
        char l_Buffer[256]{};
        std::snprintf(l_Buffer, sizeof(l_Buffer), "%s", a_Tag.Tag.c_str());

        if (ImGui::InputText("##Tag", l_Buffer, sizeof(l_Buffer)))
        {
            a_Tag.Tag = l_Buffer;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("ID: %llu", entity.GetComponent<Trinity::IDComponent>().ID);
    }

    // Transform — not removable
    DrawComponent<Trinity::TransformComponent>("Transform", entity, [](Trinity::TransformComponent& component)
    {
        DrawVec3Control("Translation", component.Translation);

        glm::vec3 l_RotationDegrees = glm::degrees(component.Rotation);
        DrawVec3Control("Rotation", l_RotationDegrees);
        component.Rotation = glm::radians(l_RotationDegrees);

        DrawVec3Control("Scale", component.Scale, 1.0f);
    }, false);

    // MeshRenderer
    DrawComponent<Trinity::MeshRendererComponent>("Mesh Renderer", entity, [](Trinity::MeshRendererComponent& component)
    {
        ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
        ImGui::LabelText("Mesh UUID", "%llu", static_cast<unsigned long long>(component.Mesh.GetUUID()));
    });

    // Material
    DrawComponent<Trinity::MaterialComponent>("Material", entity, [](Trinity::MaterialComponent& component)
    {
        const std::string l_MatName = component.Mat ? component.Mat->GetName() : "(none)";
        ImGui::LabelText("Material", "%s", l_MatName.c_str());
    });

    // Camera
    DrawComponent<Trinity::CameraComponent>("Camera", entity, [](Trinity::CameraComponent& component)
    {
        const char* l_ProjectionStrings[] = { "Perspective", "Orthographic" };
        int l_CurrentProjection = static_cast<int>(component.Projection);
        if (ImGui::Combo("Projection", &l_CurrentProjection, l_ProjectionStrings, 2))
        {
            component.Projection = static_cast<Trinity::ProjectionType>(l_CurrentProjection);
        }

        if (component.Projection == Trinity::ProjectionType::Perspective)
        {
            ImGui::DragFloat("FOV", &component.FieldOfViewDegrees, 0.5f, 1.0f, 179.0f, "%.1f deg");
            ImGui::DragFloat("Near Clip", &component.NearClip, 0.001f, 0.001f, 1.0f, "%.4f");
            ImGui::DragFloat("Far Clip", &component.FarClip, 1.0f, 1.0f, 10000.0f, "%.1f");
        }
        else
        {
            ImGui::DragFloat("Ortho Size", &component.OrthoSize, 0.1f, 0.1f, 1000.0f, "%.2f");
            ImGui::DragFloat("Near", &component.OrthoNear, 0.1f);
            ImGui::DragFloat("Far", &component.OrthoFar, 0.1f);
        }

        ImGui::Checkbox("Primary", &component.Primary);
        ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
    });

    // Light
    DrawComponent<Trinity::LightComponent>("Light", entity, [](Trinity::LightComponent& component)
    {
        const char* l_LightTypeStrings[] = { "Directional", "Point", "Spot" };
        int l_CurrentType = static_cast<int>(component.Type);
        if (ImGui::Combo("Type", &l_CurrentType, l_LightTypeStrings, 3))
        {
            component.Type = static_cast<Trinity::LightType>(l_CurrentType);
        }

        ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
        ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 1000.0f, "%.2f");

        if (component.Type != Trinity::LightType::Directional)
        {
            ImGui::DragFloat("Range", &component.Range, 0.1f, 0.0f, 1000.0f, "%.2f");
        }

        if (component.Type == Trinity::LightType::Spot)
        {
            ImGui::DragFloat("Inner Cone", &component.InnerConeAngleDegrees, 0.5f, 0.0f, 89.0f, "%.1f deg");
            ImGui::DragFloat("Outer Cone", &component.OuterConeAngleDegrees, 0.5f, 0.0f, 89.0f, "%.1f deg");
        }

        ImGui::Checkbox("Cast Shadows", &component.CastShadows);
    });

    // RigidBody
    DrawComponent<Trinity::RigidBodyComponent>("Rigid Body", entity, [](Trinity::RigidBodyComponent& component)
    {
        const char* l_TypeStrings[] = { "Static", "Dynamic", "Kinematic" };
        int l_CurrentType = static_cast<int>(component.Type);
        if (ImGui::Combo("Body Type", &l_CurrentType, l_TypeStrings, 3))
        {
            component.Type = static_cast<Trinity::RigidBodyType>(l_CurrentType);
        }

        ImGui::DragFloat("Mass", &component.Mass, 0.1f, 0.0f, 10000.0f, "%.2f");
        ImGui::DragFloat("Linear Drag", &component.LinearDrag, 0.01f, 0.0f, 1.0f, "%.3f");
        ImGui::DragFloat("Angular Drag", &component.AngularDrag, 0.01f, 0.0f, 1.0f, "%.3f");
        ImGui::Checkbox("Use Gravity", &component.UseGravity);
        ImGui::Checkbox("Is Kinematic", &component.IsKinematic);
    });

    // Script
    DrawComponent<Trinity::ScriptComponent>("Script", entity, [](Trinity::ScriptComponent& component)
    {
        char l_Buffer[512]{};
        std::snprintf(l_Buffer, sizeof(l_Buffer), "%s", component.ScriptPath.c_str());

        if (ImGui::InputText("Script Path", l_Buffer, sizeof(l_Buffer)))
        {
            component.ScriptPath = l_Buffer;
        }

        ImGui::Checkbox("Active", &component.Active);
    });
}

void ForgeLayer::DrawAddComponentMenu(Trinity::Entity entity)
{
    if (!entity.HasComponent<Trinity::MeshRendererComponent>())
    {
        if (ImGui::MenuItem("Mesh Renderer"))
        {
            entity.AddComponent<Trinity::MeshRendererComponent>();
            ImGui::CloseCurrentPopup();
        }
    }

    if (!entity.HasComponent<Trinity::MaterialComponent>())
    {
        if (ImGui::MenuItem("Material"))
        {
            entity.AddComponent<Trinity::MaterialComponent>();
            ImGui::CloseCurrentPopup();
        }
    }

    if (!entity.HasComponent<Trinity::CameraComponent>())
    {
        if (ImGui::MenuItem("Camera"))
        {
            entity.AddComponent<Trinity::CameraComponent>();
            ImGui::CloseCurrentPopup();
        }
    }

    if (!entity.HasComponent<Trinity::LightComponent>())
    {
        if (ImGui::MenuItem("Light"))
        {
            entity.AddComponent<Trinity::LightComponent>();
            ImGui::CloseCurrentPopup();
        }
    }

    if (!entity.HasComponent<Trinity::RigidBodyComponent>())
    {
        if (ImGui::MenuItem("Rigid Body"))
        {
            entity.AddComponent<Trinity::RigidBodyComponent>();
            ImGui::CloseCurrentPopup();
        }
    }

    if (!entity.HasComponent<Trinity::ScriptComponent>())
    {
        if (ImGui::MenuItem("Script"))
        {
            entity.AddComponent<Trinity::ScriptComponent>();
            ImGui::CloseCurrentPopup();
        }
    }
}

void ForgeLayer::NewScene()
{
    if (m_EditorState != EditorState::Edit)
    {
        OnStop();
    }

    LoadScene(std::make_unique<Trinity::Scene>());
    m_CurrentScenePath.clear();
}

void ForgeLayer::OpenScene(const std::string& filePath)
{
    if (m_EditorState != EditorState::Edit)
    {
        OnStop();
    }

    auto l_NewScene = std::make_unique<Trinity::Scene>();
    Trinity::SceneSerializer l_Serializer(*l_NewScene);

    if (l_Serializer.Deserialize(filePath))
    {
        LoadScene(std::move(l_NewScene));
        m_CurrentScenePath = filePath;
    }
}

void ForgeLayer::SaveScene()
{
    if (m_CurrentScenePath.empty())
    {
        SaveSceneAs();

        return;
    }

    Trinity::SceneSerializer l_Serializer(*m_ActiveScene);
    l_Serializer.Serialize(m_CurrentScenePath);
}

void ForgeLayer::SaveSceneAs()
{
    const auto l_Path = Trinity::Utilities::FileManagement::SaveFileDialog({ { "Trinity Scene", "*.trinity" } }, "trinity");
    if (l_Path.has_value())
    {
        m_CurrentScenePath = l_Path.value();
        Trinity::SceneSerializer l_Serializer(*m_ActiveScene);
        l_Serializer.Serialize(m_CurrentScenePath);
    }
}

void ForgeLayer::OnPlay()
{
    if (m_EditorState != EditorState::Edit)
    {
        return;
    }

    Trinity::SceneSerializer l_Serializer(*m_ActiveScene);
    m_SceneSnapshot = l_Serializer.SerializeToString();

    m_EditorState = EditorState::Play;
    m_SelectedEntity = Trinity::Entity{};
}

void ForgeLayer::OnStop()
{
    m_EditorState = EditorState::Edit;
    m_SelectedEntity = Trinity::Entity{};

    if (!m_SceneSnapshot.empty())
    {
        auto l_RestoredScene = std::make_unique<Trinity::Scene>();
        Trinity::SceneSerializer l_Serializer(*l_RestoredScene);
        l_Serializer.DeserializeFromString(m_SceneSnapshot);
        LoadScene(std::move(l_RestoredScene));
        m_SceneSnapshot.clear();
    }
}

void ForgeLayer::OnPause()
{
    if (m_EditorState == EditorState::Play)
    {
        m_EditorState = EditorState::Paused;
    }
    else if (m_EditorState == EditorState::Paused)
    {
        m_EditorState = EditorState::Play;
    }
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    Trinity::EventDispatcher l_Dispatcher(e);

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

    l_Dispatcher.Dispatch<Trinity::KeyPressedEvent>([this](Trinity::KeyPressedEvent& keyEvent)
    {
        if (keyEvent.GetRepeatCount() > 0)
        {
            return false;
        }

        const bool l_Ctrl = Trinity::Input::KeyDown(Trinity::Code::KeyCode::TR_KEY_LEFT_CONTROL) || Trinity::Input::KeyDown(Trinity::Code::KeyCode::TR_KEY_RIGHT_CONTROL);
        const bool l_Shift = Trinity::Input::KeyDown(Trinity::Code::KeyCode::TR_KEY_LEFT_SHIFT) || Trinity::Input::KeyDown(Trinity::Code::KeyCode::TR_KEY_RIGHT_SHIFT);

        switch (keyEvent.GetKeyCode())
        {
            case Trinity::Code::KeyCode::TR_KEY_N:
                if (l_Ctrl)
                {
                    NewScene();

                    return true;
                }
                break;

            case Trinity::Code::KeyCode::TR_KEY_O:
                if (l_Ctrl)
                {
                    const auto l_Path = Trinity::Utilities::FileManagement::OpenFileDialog({ { "Trinity Scene", "*.trinity" } });
                    if (l_Path.has_value())
                    {
                        OpenScene(l_Path.value());
                    }

                    return true;
                }
                break;

            case Trinity::Code::KeyCode::TR_KEY_S:
                if (l_Ctrl && l_Shift)
                {
                    SaveSceneAs();

                    return true;
                }

                if (l_Ctrl)
                { 
                    SaveScene();

                    return true;
                }
                break;

            default:
                break;
        }

        return false;
    });

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
        if (e.GetEventType() != Trinity::EventType::MouseRawDelta)
        {
            m_EditorCamera.OnEvent(e);
        }
    }
}