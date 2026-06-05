#include <Trinity/Core/Application.h>
#include <Trinity/Core/EntryPoint.h>
#include <Trinity/Core/Engine.h>
#include <Trinity/Core/Log.h>
#include <Trinity/Core/Event.h>
#include <Trinity/Platform/Events/KeyEvent.h>
#include <Trinity/Scene/Scene.h>
#include <Trinity/Scene/Components/NameComponent.h>
#include <Trinity/Scene/Components/HierarchyComponent.h>
#include <Trinity/Scene/Entity.h>
#include <Trinity/Scene/Components/TransformComponent.h>
#include <Trinity/Scene/Components/MeshRendererComponent.h>
#include <Trinity/Scene/Components/CameraComponent.h>

#include <cstring>

#include <imgui.h>
#include <ImGuizmo.h>
#include <Trinity/Renderer/Frontend/Camera.h>
#include <Trinity/Platform/IPlatform.h>
#include <glm/gtc/type_ptr.hpp>

namespace Trinity
{
    class ForgeApplication : public Application
    {
    public:
        ForgeApplication(const ApplicationSpecification& specification) : Application(specification)
        {

        }

        void OnInitialize() override
        {
            GetEngine().InitializeImGui();
            TR_INFO("Forge initialized");
        }

        void OnUpdate(Timestep) override
        {

        }

        void OnImGuiRender() override
        {
            ImGuizmo::BeginFrame();

            RenderDockspace();
            RenderViewport();
            RenderHierarchy();
            RenderInspector();
            RenderConsole();
        }

        void OnEvent(Event& event, bool handled) override
        {
            if (handled)
            {
                return;
            }

            EventDispatcher l_Dispatcher(event);
            l_Dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& key)
            {
                TR_INFO("Key pressed: {}", key.GetKeyCode());
                return false;
            });
        }

        void OnShutdown() override
        {
            TR_INFO("Forge shutting down");
        }

    private:
        void RenderDockspace()
        {
            ImGuiViewport* l_Viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(l_Viewport->WorkPos);
            ImGui::SetNextWindowSize(l_Viewport->WorkSize);
            ImGui::SetNextWindowViewport(l_Viewport->ID);

            ImGuiWindowFlags l_HostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("TrinityDockspaceHost", nullptr, l_HostFlags);
            ImGui::PopStyleVar(3);

            ImGuiID l_DockspaceID = ImGui::GetID("TrinityDockspace");
            ImGui::DockSpace(l_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

            ImGui::End();
        }

        void RenderConsole()
        {
            if (ImGui::Begin("Console"))
            {
                if (ImGui::Button("Clear"))
                {
                    Log::ClearMessages();
                }

                ImGui::SameLine();
                ImGui::Checkbox("Autoscroll", &m_ConsoleAutoscroll);
                ImGui::Separator();

                ImGui::BeginChild("ConsoleScroll", ImVec2(0.0f, 0.0f), 0, ImGuiWindowFlags_HorizontalScrollbar);

                Log::VisitMessages([](const LogMessage& message)
                {
                    ImVec4 l_Color(0.85f, 0.85f, 0.85f, 1.0f);
                    switch (message.Level)
                    {
                        case LogLevel::Trace: l_Color = ImVec4(0.55f, 0.55f, 0.55f, 1.0f); break;
                        case LogLevel::Info: l_Color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); break;
                        case LogLevel::Warn: l_Color = ImVec4(1.0f, 0.78f, 0.30f, 1.0f); break;
                        case LogLevel::Error: l_Color = ImVec4(1.0f, 0.40f, 0.40f, 1.0f); break;
                        case LogLevel::Critical: l_Color = ImVec4(1.0f, 0.25f, 0.25f, 1.0f); break;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, l_Color);
                    ImGui::TextUnformatted(message.Text.c_str());
                    ImGui::PopStyleColor();
                });

                if (m_ConsoleAutoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                {
                    ImGui::SetScrollHereY(1.0f);
                }

                ImGui::EndChild();
            }

            ImGui::End();
        }

        void RenderViewport()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("Viewport");

            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();

            GetEngine().SetViewportInteractive(m_ViewportHovered);

            if (m_ViewportHovered && !ImGui::GetIO().WantTextInput)
            {
                Input& l_Input = GetEngine().GetPlatform().GetInput();
                if (!l_Input.IsMouseButtonPressed(Mouse::ButtonRight))
                {
                    if (l_Input.IsKeyPressed(Key::W))
                    {
                        m_GizmoOperation = ImGuizmo::TRANSLATE;
                    }

                    if (l_Input.IsKeyPressed(Key::E))
                    {
                        m_GizmoOperation = ImGuizmo::ROTATE;
                    }

                    if (l_Input.IsKeyPressed(Key::R))
                    {
                        m_GizmoOperation = ImGuizmo::SCALE;
                    }
                }
            }

            ImVec2 l_Available = ImGui::GetContentRegionAvail();
            uint32_t l_Width = l_Available.x > 0.0f ? static_cast<uint32_t>(l_Available.x) : 0;
            uint32_t l_Height = l_Available.y > 0.0f ? static_cast<uint32_t>(l_Available.y) : 0;

            if (l_Width > 0 && l_Height > 0)
            {
                GetEngine().SetViewportSize(l_Width, l_Height);

                uint64_t l_TextureId = GetEngine().GetViewportTextureID();
                if (l_TextureId != 0)
                {
                    ImGui::Image((ImTextureID)l_TextureId, l_Available);
                    RenderGizmo(ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
                }
                else
                {
                    ImGui::TextUnformatted("Initializing viewport...");
                }
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }

        void RenderHierarchy()
        {
            ImGui::Begin("Hierarchy");

            if (GetEngine().HasScene())
            {
                Scene& l_Scene = GetEngine().GetScene();
                entt::registry& l_Registry = l_Scene.GetRegistry();

                if (m_SelectedEntity != entt::null && !l_Registry.valid(m_SelectedEntity))
                {
                    m_SelectedEntity = entt::null;
                }

                auto l_View = l_Registry.view<NameComponent>();
                for (entt::entity it_Entity : l_View)
                {
                    const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(it_Entity);
                    if (l_Hierarchy == nullptr || l_Hierarchy->Parent == entt::null)
                    {
                        RenderEntityNode(l_Scene, it_Entity);
                    }
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
                {
                    m_SelectedEntity = entt::null;
                }
            }

            ImGui::End();
        }

        void RenderEntityNode(Scene& scene, entt::entity entity)
        {
            entt::registry& l_Registry = scene.GetRegistry();
            const std::string& l_Name = l_Registry.get<NameComponent>(entity).Name;

            const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(entity);
            bool l_HasChildren = l_Hierarchy != nullptr && !l_Hierarchy->Children.empty();

            ImGuiTreeNodeFlags l_Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!l_HasChildren)
            {
                l_Flags |= ImGuiTreeNodeFlags_Leaf;
            }

            if (m_SelectedEntity == entity)
            {
                l_Flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool l_Open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity))), l_Flags, "%s", l_Name.c_str());

            if (ImGui::IsItemClicked())
            {
                m_SelectedEntity = entity;
            }

            if (l_Open)
            {
                if (l_Hierarchy != nullptr)
                {
                    for (entt::entity it_Child : l_Hierarchy->Children)
                    {
                        RenderEntityNode(scene, it_Child);
                    }
                }

                ImGui::TreePop();
            }
        }

        void RenderInspector()
        {
            ImGui::Begin("Inspector");

            if (GetEngine().HasScene() && m_SelectedEntity != entt::null)
            {
                Scene& l_Scene = GetEngine().GetScene();
                if (l_Scene.GetRegistry().valid(m_SelectedEntity))
                {
                    Entity l_Entity(m_SelectedEntity, &l_Scene);

                    NameComponent& l_NameComponent = l_Entity.GetComponent<NameComponent>();
                    char l_NameBuffer[256];
                    std::strncpy(l_NameBuffer, l_NameComponent.Name.c_str(), sizeof(l_NameBuffer) - 1);
                    l_NameBuffer[sizeof(l_NameBuffer) - 1] = '\0';
                    if (ImGui::InputText("Name", l_NameBuffer, sizeof(l_NameBuffer)))
                    {
                        l_NameComponent.Name = l_NameBuffer;
                    }

                    if (l_Entity.HasComponent<TransformComponent>())
                    {
                        ImGui::SeparatorText("Transform");
                        TransformComponent& l_Transform = l_Entity.GetComponent<TransformComponent>();
                        ImGui::DragFloat3("Translation", &l_Transform.Translation.x, 0.1f);
                        ImGui::DragFloat3("Rotation", &l_Transform.Rotation.x, 0.01f);
                        ImGui::DragFloat3("Scale", &l_Transform.Scale.x, 0.1f);
                    }

                    if (l_Entity.HasComponent<MeshRendererComponent>())
                    {
                        ImGui::SeparatorText("Mesh Renderer");
                        const MeshRendererComponent& l_MeshRenderer = l_Entity.GetComponent<MeshRendererComponent>();
                        ImGui::Text("Path: %s", l_MeshRenderer.MeshPath.empty() ? "(procedural)" : l_MeshRenderer.MeshPath.c_str());
                    }

                    if (l_Entity.HasComponent<CameraComponent>())
                    {
                        ImGui::SeparatorText("Camera");
                        CameraComponent& l_Camera = l_Entity.GetComponent<CameraComponent>();
                        ImGui::Checkbox("Primary", &l_Camera.Primary);
                    }
                }
            }

            ImGui::End();
        }

        void RenderGizmo(const ImVec2& imageMin, const ImVec2& imageSize)
        {
            if (!GetEngine().HasScene() || m_SelectedEntity == entt::null)
            {
                return;
            }

            Scene& l_Scene = GetEngine().GetScene();
            entt::registry& l_Registry = l_Scene.GetRegistry();
            if (!l_Registry.valid(m_SelectedEntity) || !l_Registry.all_of<TransformComponent>(m_SelectedEntity))
            {
                return;
            }

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

            glm::mat4 l_View = GetEngine().GetEditorCamera().GetView();
            glm::mat4 l_Projection = GetEngine().GetEditorCamera().GetProjection();
            glm::mat4 l_World = l_Scene.GetWorldMatrix(m_SelectedEntity);

            ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_World));

            if (ImGuizmo::IsUsing())
            {
                glm::mat4 l_ParentWorld(1.0f);
                const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(m_SelectedEntity);
                if (l_Hierarchy != nullptr && l_Hierarchy->Parent != entt::null)
                {
                    l_ParentWorld = l_Scene.GetWorldMatrix(l_Hierarchy->Parent);
                }

                glm::mat4 l_Local = glm::inverse(l_ParentWorld) * l_World;

                float l_Translation[3];
                float l_Rotation[3];
                float l_Scale[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(l_Local), l_Translation, l_Rotation, l_Scale);

                TransformComponent& l_Transform = l_Registry.get<TransformComponent>(m_SelectedEntity);
                l_Transform.Translation = glm::vec3(l_Translation[0], l_Translation[1], l_Translation[2]);
                l_Transform.Rotation = glm::radians(glm::vec3(l_Rotation[0], l_Rotation[1], l_Rotation[2]));
                l_Transform.Scale = glm::vec3(l_Scale[0], l_Scale[1], l_Scale[2]);
            }
        }

        bool m_ConsoleAutoscroll = true;
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        entt::entity m_SelectedEntity = entt::null;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
    };

    Application* CreateApplication(CommandLineArgs args)
    {
        ApplicationSpecification l_Specification;
        l_Specification.InternalName = "Trinity-Forge";
        l_Specification.Window.Title = "Forge";
        l_Specification.Window.Width = 1920;
        l_Specification.Window.Height = 1080;
        l_Specification.Arguments = args;

        return new ForgeApplication(l_Specification);
    }
}