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

#include <Editor/CommandHistory.h>
#include <Editor/Commands/EntityCommands.h>
#include <Editor/Commands/ComponentCommands.h>
#include <Editor/Commands/PropertyCommands.h>
#include <Editor/Commands/MeshCommands.h>

#include <Trinity/Platform/FileSystem.h>
#include <Trinity/Serialization/SceneSerializer.h>

#include <filesystem>
#include <cctype>
#include <vector>

#include <memory>
#include <functional>

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
            m_ScenePath = GetEngine().GetPlatform().GetFileSystem().Resolve(BaseDirectory::UserData, "Scenes/Demo.tscene");

            TR_INFO("Forge initialized");
        }

        void OnUpdate(Timestep) override
        {

        }

        void OnImGuiRender() override
        {
            ImGuizmo::BeginFrame();

            HandleShortcuts();
            RenderMenuBar();
            RenderDockspace();
            RenderViewport();
            RenderHierarchy();
            ProcessPendingAction();
            RenderInspector();
            ProcessDeferredComponentOp();
            RenderConsole();
            ProcessPendingFileOp();
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
        void HandleShortcuts()
        {
            ImGuiIO& l_IO = ImGui::GetIO();
            if (l_IO.WantTextInput)
            {
                return;
            }

            if (l_IO.KeyCtrl)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
                {
                    if (l_IO.KeyShift)
                    {
                        m_History.Redo();
                    }
                    else
                    {
                        m_History.Undo();
                    }
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
                {
                    m_History.Redo();
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_D, false) && m_SelectedEntity != entt::null)
                {
                    m_PendingAction = PendingAction::Duplicate;
                    m_PendingTarget = m_SelectedEntity;
                }
                else if (ImGui::IsKeyPressed(ImGuiKey_S, false))
                {
                    m_PendingFileOp = FileOp::Save;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && m_SelectedEntity != entt::null)
            {
                m_PendingAction = PendingAction::Delete;
                m_PendingTarget = m_SelectedEntity;
            }
        }

        void RenderMenuBar()
        {
            if (!ImGui::BeginMainMenuBar())
            {
                return;
            }

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    m_PendingFileOp = FileOp::Save;
                }

                if (ImGui::MenuItem("Open"))
                {
                    m_PendingFileOp = FileOp::Load;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                std::string l_UndoLabel = m_History.CanUndo() ? ("Undo " + m_History.UndoName()) : "Undo";
                if (ImGui::MenuItem(l_UndoLabel.c_str(), "Ctrl+Z", false, m_History.CanUndo()))
                {
                    m_History.Undo();
                }

                std::string l_RedoLabel = m_History.CanRedo() ? ("Redo " + m_History.RedoName()) : "Redo";
                if (ImGui::MenuItem(l_RedoLabel.c_str(), "Ctrl+Y", false, m_History.CanRedo()))
                {
                    m_History.Redo();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Entity"))
            {
                if (ImGui::MenuItem("Create Empty"))
                {
                    m_PendingAction = PendingAction::Create;
                }

                bool l_HasSelection = m_SelectedEntity != entt::null;
                if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, l_HasSelection))
                {
                    m_PendingAction = PendingAction::Duplicate;
                    m_PendingTarget = m_SelectedEntity;
                }

                if (ImGui::MenuItem("Delete", "Del", false, l_HasSelection))
                {
                    m_PendingAction = PendingAction::Delete;
                    m_PendingTarget = m_SelectedEntity;
                }

                ImGui::EndMenu();
            }

            std::string l_Title = m_SceneName + (m_History.IsDirty() ? " *" : "");
            float l_TitleWidth = ImGui::CalcTextSize(l_Title.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - l_TitleWidth - 12.0f);
            ImGui::TextUnformatted(l_Title.c_str());

            ImGui::EndMainMenuBar();
        }

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

                if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
                {
                    if (ImGui::MenuItem("Create Empty"))
                    {
                        m_PendingAction = PendingAction::Create;
                    }

                    ImGui::EndPopup();
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

            if (ImGui::BeginDragDropSource())
            {
                entt::entity l_Payload = entity;
                ImGui::SetDragDropPayload("TRINITY_ENTITY", &l_Payload, sizeof(l_Payload));
                ImGui::TextUnformatted(l_Name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* l_Accepted = ImGui::AcceptDragDropPayload("TRINITY_ENTITY"))
                {
                    entt::entity l_Dragged = *static_cast<const entt::entity*>(l_Accepted->Data);
                    m_PendingAction = PendingAction::Reparent;
                    m_PendingTarget = l_Dragged;
                    m_PendingParent = entity;
                }

                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginPopupContextItem())
            {
                m_SelectedEntity = entity;

                if (ImGui::MenuItem("Duplicate"))
                {
                    m_PendingAction = PendingAction::Duplicate;
                    m_PendingTarget = entity;
                }

                bool l_HasParent = l_Hierarchy != nullptr && l_Hierarchy->Parent != entt::null;
                if (ImGui::MenuItem("Unparent", nullptr, false, l_HasParent))
                {
                    m_PendingAction = PendingAction::Reparent;
                    m_PendingTarget = entity;
                    m_PendingParent = entt::null;
                }

                if (ImGui::MenuItem("Delete"))
                {
                    m_PendingAction = PendingAction::Delete;
                    m_PendingTarget = entity;
                }

                ImGui::EndPopup();
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

                    bool l_NameChanged = ImGui::InputText("Name", l_NameBuffer, sizeof(l_NameBuffer));
                    if (ImGui::IsItemActivated())
                    {
                        m_RenameOldName = l_NameComponent.Name;
                    }

                    if (l_NameChanged)
                    {
                        l_NameComponent.Name = l_NameBuffer;
                    }

                    if (ImGui::IsItemDeactivatedAfterEdit())
                    {
                        if (l_NameComponent.Name != m_RenameOldName)
                        {
                            m_History.Execute(std::make_unique<RenameEntityCommand>(l_Scene, static_cast<uint64_t>(l_Entity.GetUUID()), m_RenameOldName, l_NameComponent.Name));
                        }
                    }
                    else if (ImGui::IsItemDeactivated())
                    {
                        l_NameComponent.Name = m_RenameOldName;
                    }

                    if (l_Entity.HasComponent<TransformComponent>())
                    {
                        ImGui::SeparatorText("Transform");
                        TransformComponent& l_Transform = l_Entity.GetComponent<TransformComponent>();
                        uint64_t l_TransformUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                        DragTransformField(l_Scene, l_TransformUUID, "Translation", l_Transform, l_Transform.Translation, 0.1f);
                        DragTransformField(l_Scene, l_TransformUUID, "Rotation", l_Transform, l_Transform.Rotation, 0.01f);
                        DragTransformField(l_Scene, l_TransformUUID, "Scale", l_Transform, l_Transform.Scale, 0.1f);
                    }

                    if (l_Entity.HasComponent<MeshRendererComponent>())
                    {
                        ImGui::SeparatorText("Mesh Renderer");
                        MeshRendererComponent& l_MeshRenderer = l_Entity.GetComponent<MeshRendererComponent>();
                        uint64_t l_MeshUUID = static_cast<uint64_t>(l_Entity.GetUUID());

                        if (!m_MeshesScanned)
                        {
                            ScanAvailableMeshes();
                            m_MeshesScanned = true;
                        }

                        std::string l_CurrentLabel = l_MeshRenderer.MeshPath.empty() ? "(procedural cube)" : l_MeshRenderer.MeshPath;
                        if (ImGui::BeginCombo("Mesh", l_CurrentLabel.c_str()))
                        {
                            if (ImGui::Selectable("(procedural cube)", l_MeshRenderer.MeshPath.empty()) && !l_MeshRenderer.MeshPath.empty())
                            {
                                m_History.Execute(std::make_unique<SetMeshCommand>(l_Scene, GetEngine().GetMeshLibrary(), l_MeshUUID, std::string()));
                            }

                            for (const std::string& it_Path : m_AvailableMeshes)
                            {
                                bool l_Selected = it_Path == l_MeshRenderer.MeshPath;
                                if (ImGui::Selectable(it_Path.c_str(), l_Selected) && !l_Selected)
                                {
                                    m_History.Execute(std::make_unique<SetMeshCommand>(l_Scene, GetEngine().GetMeshLibrary(), l_MeshUUID, it_Path));
                                }
                            }

                            ImGui::EndCombo();
                        }

                        ImGui::SameLine();
                        if (ImGui::SmallButton("Refresh##Meshes"))
                        {
                            ScanAvailableMeshes();
                        }

                        if (ImGui::SmallButton("Remove##MeshRenderer"))
                        {
                            m_PendingComponentOp = [this, l_MeshUUID]()
                            {
                                Scene& l_OpScene = GetEngine().GetScene();
                                m_History.Execute(std::make_unique<RemoveComponentCommand<MeshRendererComponent>>(l_OpScene, l_MeshUUID, "Mesh Renderer"));
                            };
                        }
                    }

                    if (l_Entity.HasComponent<CameraComponent>())
                    {
                        ImGui::SeparatorText("Camera");
                        CameraComponent& l_Camera = l_Entity.GetComponent<CameraComponent>();
                        if (ImGui::Checkbox("Primary", &l_Camera.Primary))
                        {
                            m_History.Execute(std::make_unique<SetCameraPrimaryCommand>(l_Scene, static_cast<uint64_t>(l_Entity.GetUUID()), !l_Camera.Primary, l_Camera.Primary));
                        }

                        if (ImGui::SmallButton("Remove##Camera"))
                        {
                            uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());
                            m_PendingComponentOp = [this, l_TargetUUID]()
                            {
                                Scene& l_OpScene = GetEngine().GetScene();
                                m_History.Execute(std::make_unique<RemoveComponentCommand<CameraComponent>>(l_OpScene, l_TargetUUID, "Camera"));
                            };
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::Button("Add Component"))
                    {
                        ImGui::OpenPopup("AddComponentPopup");
                    }

                    if (ImGui::BeginPopup("AddComponentPopup"))
                    {
                        uint64_t l_TargetUUID = static_cast<uint64_t>(l_Entity.GetUUID());

                        if (!l_Entity.HasComponent<MeshRendererComponent>() && ImGui::MenuItem("Mesh Renderer"))
                        {
                            m_PendingComponentOp = [this, l_TargetUUID]()
                            {
                                Scene& l_OpScene = GetEngine().GetScene();
                                m_History.Execute(std::make_unique<AddComponentCommand<MeshRendererComponent>>(l_OpScene, l_TargetUUID, "Mesh Renderer"));
                            };
                        }

                        if (!l_Entity.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
                        {
                            m_PendingComponentOp = [this, l_TargetUUID]()
                            {
                                Scene& l_OpScene = GetEngine().GetScene();
                                m_History.Execute(std::make_unique<AddComponentCommand<CameraComponent>>(l_OpScene, l_TargetUUID, "Camera"));
                            };
                        }

                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::End();
        }

        void DragTransformField(Scene& scene, uint64_t uuid, const char* label, TransformComponent& transform, glm::vec3& value, float speed)
        {
            ImGui::DragFloat3(label, &value.x, speed);

            if (ImGui::IsItemActivated())
            {
                m_TransformEditOld = transform;
            }

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                m_History.Execute(std::make_unique<SetTransformCommand>(scene, uuid, m_TransformEditOld, transform));
            }
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

            TransformComponent l_PreEdit = l_Registry.get<TransformComponent>(m_SelectedEntity);

            ImGuizmo::Manipulate(glm::value_ptr(l_View), glm::value_ptr(l_Projection), m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(l_World));

            bool l_IsUsing = ImGuizmo::IsUsing();

            if (l_IsUsing)
            {
                if (!m_GizmoWasUsing)
                {
                    m_GizmoStartTransform = l_PreEdit;
                }

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

            if (!l_IsUsing && m_GizmoWasUsing)
            {
                TransformComponent l_Current = l_Registry.get<TransformComponent>(m_SelectedEntity);
                if (l_Current.Translation != m_GizmoStartTransform.Translation || l_Current.Rotation != m_GizmoStartTransform.Rotation || l_Current.Scale != m_GizmoStartTransform.Scale)
                {
                    uint64_t l_UUID = static_cast<uint64_t>(Entity(m_SelectedEntity, &l_Scene).GetUUID());
                    m_History.Execute(std::make_unique<SetTransformCommand>(l_Scene, l_UUID, m_GizmoStartTransform, l_Current));
                }
            }

            m_GizmoWasUsing = l_IsUsing;
        }

        void ScanAvailableMeshes()
        {
            m_AvailableMeshes.clear();

            std::filesystem::path l_AssetsDir = GetEngine().GetPlatform().GetFileSystem().Resolve(BaseDirectory::Executable, "Assets");

            std::error_code l_Error;
            if (!std::filesystem::exists(l_AssetsDir, l_Error))
            {
                return;
            }

            for (const std::filesystem::directory_entry& it_Entry : std::filesystem::directory_iterator(l_AssetsDir, l_Error))
            {
                if (!it_Entry.is_regular_file())
                {
                    continue;
                }

                std::string l_Extension = it_Entry.path().extension().string();
                for (char& it_Char : l_Extension)
                {
                    it_Char = static_cast<char>(std::tolower(static_cast<unsigned char>(it_Char)));
                }

                if (l_Extension == ".obj" || l_Extension == ".fbx" || l_Extension == ".gltf" || l_Extension == ".glb" || l_Extension == ".dae" || l_Extension == ".3ds")
                {
                    m_AvailableMeshes.push_back("Assets/" + it_Entry.path().filename().string());
                }
            }
        }

        void ProcessDeferredComponentOp()
        {
            if (m_PendingComponentOp)
            {
                std::function<void()> l_Op = std::move(m_PendingComponentOp);
                m_PendingComponentOp = nullptr;
                l_Op();
            }
        }

        void ProcessPendingFileOp()
        {
            if (m_PendingFileOp == FileOp::None || !GetEngine().HasScene())
            {
                m_PendingFileOp = FileOp::None;

                return;
            }

            Scene& l_Scene = GetEngine().GetScene();

            if (m_PendingFileOp == FileOp::Save)
            {
                if (SceneSerializer::Serialize(l_Scene, m_ScenePath, m_SceneName))
                {
                    m_History.MarkSaved();
                }
            }
            else if (m_PendingFileOp == FileOp::Load)
            {
                l_Scene.GetRegistry().clear();
                m_SelectedEntity = entt::null;
                SceneSerializer::Deserialize(l_Scene, GetEngine().GetMeshLibrary(), m_ScenePath);
                m_History.Clear();
            }

            m_PendingFileOp = FileOp::None;
        }

        bool IsAncestorOf(Scene& scene, entt::entity ancestor, entt::entity node)
        {
            entt::registry& l_Registry = scene.GetRegistry();
            entt::entity l_Current = node;
            while (l_Current != entt::null)
            {
                if (l_Current == ancestor)
                {
                    return true;
                }

                const HierarchyComponent* l_Hierarchy = l_Registry.try_get<HierarchyComponent>(l_Current);
                l_Current = l_Hierarchy != nullptr ? l_Hierarchy->Parent : entt::null;
            }

            return false;
        }

        void ProcessPendingAction()
        {
            if (m_PendingAction == PendingAction::None || !GetEngine().HasScene())
            {
                m_PendingAction = PendingAction::None;
                m_PendingTarget = entt::null;

                return;
            }

            Scene& l_Scene = GetEngine().GetScene();

            if (m_PendingAction == PendingAction::Create)
            {
                std::unique_ptr<CreateEntityCommand> l_Command = std::make_unique<CreateEntityCommand>(l_Scene, "Entity");
                CreateEntityCommand* l_Raw = l_Command.get();
                m_History.Execute(std::move(l_Command));

                Entity l_Created = FindEntityByUUID(l_Scene, l_Raw->GetResultUUID());
                m_SelectedEntity = l_Created.IsValid() ? l_Created.GetHandle() : entt::null;
            }
            else if (m_PendingAction == PendingAction::Duplicate)
            {
                if (m_PendingTarget != entt::null && l_Scene.GetRegistry().valid(m_PendingTarget))
                {
                    std::unique_ptr<DuplicateEntityCommand> l_Command = std::make_unique<DuplicateEntityCommand>(l_Scene, GetEngine().GetMeshLibrary(), m_PendingTarget);
                    DuplicateEntityCommand* l_Raw = l_Command.get();
                    m_History.Execute(std::move(l_Command));

                    Entity l_Duplicate = FindEntityByUUID(l_Scene, l_Raw->GetResultUUID());
                    m_SelectedEntity = l_Duplicate.IsValid() ? l_Duplicate.GetHandle() : entt::null;
                }
            }
            else if (m_PendingAction == PendingAction::Delete)
            {
                if (m_PendingTarget != entt::null && l_Scene.GetRegistry().valid(m_PendingTarget))
                {
                    m_History.Execute(std::make_unique<DeleteEntityCommand>(l_Scene, GetEngine().GetMeshLibrary(), m_PendingTarget));

                    if (m_SelectedEntity == m_PendingTarget)
                    {
                        m_SelectedEntity = entt::null;
                    }
                }
            }
            else if (m_PendingAction == PendingAction::Reparent)
            {
                if (m_PendingTarget != entt::null && l_Scene.GetRegistry().valid(m_PendingTarget))
                {
                    entt::entity l_NewParent = m_PendingParent;
                    bool l_Valid = l_NewParent == entt::null || l_Scene.GetRegistry().valid(l_NewParent);
                    if (l_Valid && IsAncestorOf(l_Scene, m_PendingTarget, l_NewParent))
                    {
                        l_Valid = false;
                    }

                    if (l_Valid)
                    {
                        uint64_t l_ChildUUID = static_cast<uint64_t>(Entity(m_PendingTarget, &l_Scene).GetUUID());
                        uint64_t l_ParentUUID = l_NewParent != entt::null ? static_cast<uint64_t>(Entity(l_NewParent, &l_Scene).GetUUID()) : 0;
                        m_History.Execute(std::make_unique<SetParentCommand>(l_Scene, l_ChildUUID, l_ParentUUID));
                    }
                }
            }

            m_PendingAction = PendingAction::None;
            m_PendingTarget = entt::null;
            m_PendingParent = entt::null;
        }

        enum class PendingAction
        {
            None,
            Create,
            Duplicate,
            Delete,
            Reparent
        };

        enum class FileOp
        {
            None,
            Save,
            Load
        };

        bool m_ConsoleAutoscroll = true;
        bool m_ViewportFocused = false;
        bool m_ViewportHovered = false;
        entt::entity m_SelectedEntity = entt::null;
        ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
        CommandHistory m_History;
        PendingAction m_PendingAction = PendingAction::None;
        entt::entity m_PendingTarget = entt::null;
        entt::entity m_PendingParent = entt::null;
        std::string m_RenameOldName;
        std::function<void()> m_PendingComponentOp;
        bool m_GizmoWasUsing = false;
        TransformComponent m_GizmoStartTransform;
        TransformComponent m_TransformEditOld;
        std::vector<std::string> m_AvailableMeshes;
        bool m_MeshesScanned = false;
        FileOp m_PendingFileOp = FileOp::None;
        std::filesystem::path m_ScenePath;
        std::string m_SceneName = "Demo";
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