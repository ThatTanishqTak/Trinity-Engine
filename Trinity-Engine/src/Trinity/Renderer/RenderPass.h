#pragma once

#include <cstdint>

namespace Trinity
{
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Shutdown() = 0;
        virtual void Recreate(uint32_t width, uint32_t height) = 0;
        virtual const char* GetName() const = 0;
    };
}