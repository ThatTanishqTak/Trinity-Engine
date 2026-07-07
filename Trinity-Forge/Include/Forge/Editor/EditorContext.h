#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include <Forge/Editor/CommandHistory.h>

namespace Trinity
{
    enum class PendingAction
    {
        None,
        Create,
        Duplicate,
        Delete,
        Reparent
    };

    enum class PendingFileOp
    {
        None,
        Save,
        Load
    };

    struct EditorContext
    {
        // Primary (last-clicked) selection; always a member of Selection when non-null.
        entt::entity SelectedEntity = entt::null;
        // Full selection set in click order. Single-target consumers keep using SelectedEntity.
        std::vector<entt::entity> Selection;
        CommandHistory History;

        bool IsSelected(entt::entity entity) const
        {
            return std::find(Selection.begin(), Selection.end(), entity) != Selection.end();
        }

        void Select(entt::entity entity)
        {
            Selection.clear();
            if (entity != entt::null)
            {
                Selection.push_back(entity);
            }

            SelectedEntity = entity;
        }

        void AddToSelection(entt::entity entity)
        {
            if (entity == entt::null)
            {
                return;
            }

            if (!IsSelected(entity))
            {
                Selection.push_back(entity);
            }

            SelectedEntity = entity;
        }

        void ToggleSelection(entt::entity entity)
        {
            if (entity == entt::null)
            {
                return;
            }

            auto l_Found = std::find(Selection.begin(), Selection.end(), entity);
            if (l_Found == Selection.end())
            {
                Selection.push_back(entity);
                SelectedEntity = entity;

                return;
            }

            Selection.erase(l_Found);
            if (SelectedEntity == entity)
            {
                SelectedEntity = Selection.empty() ? entt::null : Selection.back();
            }
        }

        void Deselect(entt::entity entity)
        {
            auto l_Found = std::find(Selection.begin(), Selection.end(), entity);
            if (l_Found != Selection.end())
            {
                Selection.erase(l_Found);
            }

            if (SelectedEntity == entity)
            {
                SelectedEntity = Selection.empty() ? entt::null : Selection.back();
            }
        }

        void ClearSelection()
        {
            Selection.clear();
            SelectedEntity = entt::null;
        }

        void PruneSelection(const entt::registry& registry)
        {
            Selection.erase(std::remove_if(Selection.begin(), Selection.end(), [&registry](entt::entity it_Entity)
                {
                    return !registry.valid(it_Entity);
                }), Selection.end());

            if (SelectedEntity != entt::null && !registry.valid(SelectedEntity))
            {
                SelectedEntity = Selection.empty() ? entt::null : Selection.back();
            }
        }

        PendingAction Action = PendingAction::None;
        entt::entity ActionTarget = entt::null;
        entt::entity ActionParent = entt::null;

        std::function<void()> ComponentOp;

        PendingFileOp FileOp = PendingFileOp::None;

        std::filesystem::path ScenePath;
        std::string SceneName = "Demo";

        bool ResetLayout = false;
        bool ShowContentDrawer = false;
        bool ShowConsoleDrawer = false;
        bool DrawerToggled = false;
        bool PlayMode = false;
        bool ShowRenderGraph = false;

        float ChromeTop = 0.0f;
        float ChromeBottom = 0.0f;
    };
}