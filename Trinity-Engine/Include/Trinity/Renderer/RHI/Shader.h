#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    struct ShaderDescription
    {
        ShaderStage Stage = ShaderStage::None;
        
        std::vector<uint8_t> Bytecode;
        
        std::string EntryPoint = "main";
        std::string DebugName;
    };
}