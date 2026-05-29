#pragma once

#include <cstdint>
#include <string>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    struct BufferDescription
    {
        uint64_t Size = 0;
        
        BufferUsage Usage = BufferUsage::None;
        MemoryUsage Memory = MemoryUsage::GpuOnly;
        
        const void* InitialData = nullptr;
        
        std::string DebugName;
    };
}