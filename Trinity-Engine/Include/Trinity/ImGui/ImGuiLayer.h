#pragma once

#include <cstdint>

#include <Trinity/ImGui/IImGuiPlatformBackend.h>
#include <Trinity/ImGui/IImGuiRenderBackend.h>

namespace Trinity
{
    class CommandList;

    class ImGuiLayer
    {
    public:
        ImGuiLayer() = default;
        ~ImGuiLayer();

        ImGuiLayer(const ImGuiLayer&) = delete;
        ImGuiLayer& operator=(const ImGuiLayer&) = delete;

        bool Initialize(IImGuiPlatformBackend& platform, IImGuiRenderBackend& render, uint32_t framesInFlight, Format colorFormat);
        void Shutdown();

        void BeginFrame();
        void EndFrame(CommandList& commandList);

        bool IsInitialized() const { return m_Initialized; }

    private:
        IImGuiPlatformBackend* m_Platform = nullptr;
        IImGuiRenderBackend* m_Render = nullptr;
        bool m_Initialized = false;
    };
}