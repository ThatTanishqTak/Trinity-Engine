#pragma once

namespace Trinity
{
    class IImGuiPlatformBackend
    {
    public:
        virtual ~IImGuiPlatformBackend() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void NewFrame() = 0;
    };
}