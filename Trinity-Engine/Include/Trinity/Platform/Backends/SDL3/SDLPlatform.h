#pragma once

#include <memory>

#include <Trinity/Platform/IPlatform.h>

namespace Trinity
{
    class SDLPlatform : public IPlatform
    {
    public:
        SDLPlatform();
        ~SDLPlatform() override;

        bool Initialize() override;
        void Shutdown() override;

        void PollEvents() override;

        Window& CreateWindow(const WindowProperties& properties) override;
        Window& GetWindow() override;
        bool HasWindow() const override;

        Input& GetInput() override;
        Gamepad& GetGamepad() override;
        FileSystem& GetFileSystem() override;

        PlatformType GetType() const override { return m_Type; }

    private:
        PlatformType m_Type;
        bool m_Initialized = false;

        std::unique_ptr<Window> m_Window;
        std::unique_ptr<Input> m_Input;
        std::unique_ptr<Gamepad> m_Gamepad;
        std::unique_ptr<FileSystem> m_FileSystem;
    };
}