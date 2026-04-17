#pragma once

#include <entt/entt.hpp>

namespace Trinity
{
    class Scene;
}

enum class EditorState : uint8_t
{
    Edit = 0,
    Play,
    Pause
};

struct SelectionContext
{
    entt::entity    SelectedEntity = entt::null;
    Trinity::Scene* ActiveScene    = nullptr;
    EditorState     State          = EditorState::Edit;
};
