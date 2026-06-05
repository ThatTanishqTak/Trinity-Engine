#pragma once

#include <cstdint>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>

namespace Trinity
{
    class CommandList;

    class IImGuiRenderBackend
    {
    public:
        virtual ~IImGuiRenderBackend() = default;

        virtual bool Initialize(uint32_t framesInFlight, Format colorFormat) = 0;
        virtual void Shutdown() = 0;

        virtual void NewFrame() = 0;
        virtual void RecordDrawData(CommandList& commandList) = 0;

        virtual uint64_t RegisterTexture(TextureHandle texture) = 0;
        virtual void UnregisterTexture(uint64_t textureID) = 0;
    };
}