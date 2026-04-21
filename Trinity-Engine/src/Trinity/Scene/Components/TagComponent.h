#pragma once

#include <string>

namespace Trinity
{
    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        explicit TagComponent(std::string tag) : Tag(std::move(tag))
        {

        }
    };
}