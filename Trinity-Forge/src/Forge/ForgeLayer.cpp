#include "Forge/ForgeLayer.h"

#include "Trinity/Application/Application.h"
#include "Trinity/ImGui/ImGuiLayer.h"
#include "Trinity/Events/Event.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Scene/SceneSerializer.h"
#include "Trinity/Platform/Input/Desktop/DesktopInput.h"
#include "Trinity/Platform/Input/Desktop/DesktopInputCodes.h"

#include <imgui.h>
#include <ImGuizmo.h>

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{
}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    m_SelectionContext.ActiveScene = &m_Scene;

    m_PanelManager.Initialize();

    m_ViewportPanel = m_PanelManager.RegisterPanel<Forge::ViewportPanel>("Viewport", &m_SelectionContext);
    m_HierarchyPanel = m_PanelManager.RegisterPanel<Forge::SceneHierarchyPanel>("Scene Hierarchy", &m_SelectionContext);
    m_InspectorPanel = m_PanelManager.RegisterPanel<Forge::InspectorPanel>("Inspector", &m_SelectionContext);
    m_ContentPanel = m_PanelManager.RegisterPanel<Forge::ContentBrowserPanel>("Content Browser");
    m_StatsPanel = m_PanelManager.RegisterPanel<Forge::RendererStatsPanel>("Renderer Stats");
    m_LogPanel = m_PanelManager.RegisterPanel<Forge::LogPanel>("Log");

    Trinity::ImGuiLayer::Get().SetMenuBarCallback([this]()
    {
        RenderMenuBar();
    });
}

void ForgeLayer::OnShutdown()
{
    m_PanelManager.Shutdown();
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    // Keyboard shortcuts
    const bool l_Ctrl = Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_LEFT_CONTROL);
    const bool l_Shift = Trinity::DesktopInput::KeyDown(Trinity::Code::KeyCode::TR_KEY_LEFT_SHIFT);

    if (l_Ctrl)
    {
        if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_N))
        {
            NewScene();
        }
        else if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_O))
        {
            m_ShowOpenSceneModal = true;
        }
        else if (Trinity::DesktopInput::KeyPressed(Trinity::Code::KeyCode::TR_KEY_S))
        {
            if (l_Shift || m_CurrentScenePath.empty())
            {
                m_ShowSaveSceneAsModal = true;
            }
            else
            {
                SaveScene();
            }
        }
    }

    // Detect state transitions and react
    if (m_SelectionContext.State != m_LastEditorState)
    {
        const EditorState l_Previous = m_LastEditorState;
        const EditorState l_Current = m_SelectionContext.State;

        if (l_Current == EditorState::Play && l_Previous == EditorState::Edit)
        {
            m_SceneSnapshot = Trinity::SceneSerializer::SerializeToString(m_Scene);
        }
        else if (l_Current == EditorState::Edit && (l_Previous == EditorState::Play || l_Previous == EditorState::Pause))
        {
            if (!m_SceneSnapshot.empty())
            {
                m_Scene = Trinity::Scene();
                Trinity::SceneSerializer::DeserializeFromString(m_Scene, m_SceneSnapshot);
                m_SelectionContext.ActiveScene = &m_Scene;
            }
            m_SelectionContext.SelectedEntity = entt::null;
        }

        m_LastEditorState = l_Current;
    }

    m_PanelManager.UpdatePanels(deltaTime);
}

void ForgeLayer::OnRender()
{
    m_PanelManager.PreRenderPanels();
}

void ForgeLayer::OnImGuiRender()
{
    ImGuizmo::BeginFrame();

    m_PanelManager.RenderPanels();

    RenderFileDialogModals();

    if (m_ShowAboutPopup)
    {
        ImGui::OpenPopup("About Trinity Engine");
        m_ShowAboutPopup = false;
    }

    ImVec2 l_Center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("About Trinity Engine", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Trinity Engine");
        ImGui::Separator();
        ImGui::Text("Vulkan-backed game engine with deferred rendering.");
        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;
}

void ForgeLayer::NewScene()
{
    m_Scene = Trinity::Scene("Untitled");
    m_CurrentScenePath.clear();
    m_SceneSnapshot.clear();
    m_SelectionContext.SelectedEntity = entt::null;
    m_SelectionContext.ActiveScene = &m_Scene;
}

void ForgeLayer::OpenScene(const std::string& filepath)
{
    Trinity::Scene l_NewScene;
    if (Trinity::SceneSerializer::Deserialize(l_NewScene, filepath))
    {
        m_Scene = std::move(l_NewScene);
        m_CurrentScenePath = filepath;
        m_SceneSnapshot.clear();
        m_SelectionContext.SelectedEntity = entt::null;
        m_SelectionContext.ActiveScene = &m_Scene;
    }
}

void ForgeLayer::SaveScene()
{
    if (!m_CurrentScenePath.empty())
    {
        Trinity::SceneSerializer::Serialize(m_Scene, m_CurrentScenePath);
    }
}

void ForgeLayer::SaveSceneAs(const std::string& filepath)
{
    Trinity::SceneSerializer::Serialize(m_Scene, filepath);
    m_CurrentScenePath = filepath;
}

void ForgeLayer::RenderFileDialogModals()
{
    if (m_ShowOpenSceneModal)
    {
        ImGui::OpenPopup("Open Scene");
        m_ShowOpenSceneModal = false;
    }

    if (m_ShowSaveSceneAsModal)
    {
        ImGui::OpenPopup("Save Scene As");
        m_ShowSaveSceneAsModal = false;
    }

    const ImVec2 l_Center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Scene path (.trinity):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##openpath", m_FilePathBuffer, sizeof(m_FilePathBuffer));

        if (ImGui::Button("Open", ImVec2(120.0f, 0.0f)))
        {
            OpenScene(m_FilePathBuffer);
            m_FilePathBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
        {
            m_FilePathBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(l_Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Scene path (.trinity):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##savepath", m_FilePathBuffer, sizeof(m_FilePathBuffer));

        if (ImGui::Button("Save", ImVec2(120.0f, 0.0f)))
        {
            SaveSceneAs(m_FilePathBuffer);
            m_FilePathBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
        {
            m_FilePathBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ForgeLayer::RenderMenuBar()
{
    // File
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Scene", "Ctrl+N"))
        {
            NewScene();
        }

        if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
        {
            m_ShowOpenSceneModal = true;
        }

        ImGui::Separator();

        const bool l_HasPath = !m_CurrentScenePath.empty();

        if (!l_HasPath)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            SaveScene();
        }

        if (!l_HasPath)
        {
            ImGui::EndDisabled();
        }

        if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
        {
            m_ShowSaveSceneAsModal = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
        {
            Trinity::Application::Get().Close();
        }

        ImGui::EndMenu();
    }

    // Edit
    if (ImGui::BeginMenu("Edit"))
    {
        ImGui::BeginDisabled();
        ImGui::MenuItem("Undo", "Ctrl+Z");
        ImGui::MenuItem("Redo", "Ctrl+Y");
        ImGui::EndDisabled();

        ImGui::EndMenu();
    }

    // View
    if (ImGui::BeginMenu("View"))
    {
        m_PanelManager.RenderViewMenu();
        ImGui::EndMenu();
    }

    // Renderer
    if (ImGui::BeginMenu("Renderer"))
    {
        ImGui::Text("Backend: %s", Trinity::Renderer::GetBackend() == Trinity::RendererBackend::Vulkan ? "Vulkan" : "Unknown");

        ImGui::Separator();

        ImGui::BeginDisabled();
        ImGui::MenuItem("Show GeometryBuffer Albedo");
        ImGui::MenuItem("Show GeometryBuffer Normal");
        ImGui::MenuItem("Show GeometryBuffer MRA");
        ImGui::MenuItem("Show Shadow Map");
        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::MenuItem("Renderer Stats", nullptr, &m_StatsPanel->GetOpenState());

        ImGui::EndMenu();
    }

    // Help
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("About Trinity Engine"))
        {
            m_ShowAboutPopup = true;
        }

        ImGui::EndMenu();
    }
}