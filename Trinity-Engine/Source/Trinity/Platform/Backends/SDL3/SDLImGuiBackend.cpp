#include <Trinity/Platform/Backends/SDL3/SDLImGuiBackend.h>

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    SDLImGuiBackend::SDLImGuiBackend(SDL_Window* window) : m_Window(window)
    {

    }

    SDLImGuiBackend::~SDLImGuiBackend()
    {
        Shutdown();
    }

    bool SDLImGuiBackend::Initialize()
    {
        if (m_Initialized)
        {
            return true;
        }

        if (m_Window == nullptr)
        {


            return false;
        }

        if (!ImGui_ImplSDL3_InitForVulkan(m_Window))
        {


            return false;
        }

        m_Initialized = true;


        return true;
    }

    void SDLImGuiBackend::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplSDL3_Shutdown();
        m_Initialized = false;
    }

    void SDLImGuiBackend::NewFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplSDL3_NewFrame();
    }

    void SDLImGuiBackend::ProcessEvent(const SDL_Event& event)
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}