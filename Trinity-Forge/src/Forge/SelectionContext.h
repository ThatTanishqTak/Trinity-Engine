#pragma once

#include <entt/entt.hpp>

namespace Trinity
{
    class Scene;
}

struct SelectionContext
{
    entt::entity    SelectedEntity = entt::null;
    Trinity::Scene* ActiveScene    = nullptr;
};
