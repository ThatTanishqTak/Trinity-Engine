#pragma once

#include <filesystem>
#include <functional>
#include <string>

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
        entt::entity SelectedEntity = entt::null;
        CommandHistory History;

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