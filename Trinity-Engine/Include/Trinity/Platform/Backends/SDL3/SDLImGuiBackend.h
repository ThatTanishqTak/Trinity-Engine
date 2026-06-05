#pragma once

#include <Trinity/ImGui/IImGuiPlatformBackend.h>

struct SDL_Window;
union SDL_Event;

namespace Trinity
{
    class SDLImGuiBackend : public IImGuiPlatformBackend
    {
    public:
        explicit SDLImGuiBackend(SDL_Window* window);
        ~SDLImGuiBackend() override;

        SDLImGuiBackend(const SDLImGuiBackend&) = delete;
        SDLImGuiBackend& operator=(const SDLImGuiBackend&) = delete;

        bool Initialize() override;
        void Shutdown() override;

        void NewFrame() override;

        void ProcessEvent(const SDL_Event& event);

    private:
        SDL_Window* m_Window = nullptr;
        bool m_Initialized = false;
    };
}