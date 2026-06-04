#pragma once

#include <vector>

#include <entt/entt.hpp>

namespace Trinity
{
    struct HierarchyComponent
    {
        entt::entity Parent{ entt::null };
        std::vector<entt::entity> Children;
    };
}