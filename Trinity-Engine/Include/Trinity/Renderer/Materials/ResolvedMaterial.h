#pragma once

#include <string>

#include <Trinity/Renderer/Materials/MaterialParameter.h>

namespace Trinity
{
    struct ResolvedMaterial
    {
        std::string Shader = "Mesh";
        MaterialParameterMap Parameters;
    };
}