#pragma once

#include <cstdint>
#include <string>

#include <Trinity/Renderer/RHI/GraphicsTypes.h>
#include <Trinity/Renderer/RHI/Handle.h>
#include <Trinity/Platform/PlatformTypes.h>

namespace Trinity
{
    enum class PresentMode
    {
        Immediate = 0,
        Mailbox,
        Fifo,
        FifoRelaxed
    };

    struct SwapchainDescription
    {
        NativeWindowHandle Window;
        
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t ImageCount = 3;

        Format PreferredFormat = Format::BGRA8_SRGB;
        PresentMode PreferredPresentMode = PresentMode::Fifo;
        
        bool VSync = true;
        
        std::string DebugName;
    };

    struct FrameInfo
    {
        TextureHandle BackBuffer;
        
        uint32_t ImageIndex = 0;
    };

    class Swapchain
    {
    public:
        virtual ~Swapchain() = default;

        virtual bool AcquireNextImage(FrameInfo& outFrame) = 0;
        virtual void Present() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual Format GetFormat() const = 0;
        virtual uint32_t GetImageCount() const = 0;
    };
}