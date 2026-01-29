#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace Engine
{
    class Window;

    namespace Render
    {
        class Camera;
    }

    enum class API
    {
        None = 0,
        Vulkan
    };

    enum class CommandType
    {
        Clear = 0,
        DrawTriangle,
        DrawCube
    };

    struct Command
    {
        CommandType Type = CommandType::Clear;
        glm::vec4 Color = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f); // Used for Clear
    };

    class RendererAPI
    {
    public:
        virtual ~RendererAPI() = default;

        virtual API GetAPI() const = 0;

        virtual void Initialize(Window* window) = 0;
        virtual void Shutdown() = 0;

        virtual bool BeginFrame() = 0;
        virtual void Execute(const std::vector<Command>& commandList) = 0;
        virtual void EndFrame() = 0;

        virtual void SetActiveCamera(Render::Camera* camera) = 0;

        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void WaitIdle() = 0;
    };
}