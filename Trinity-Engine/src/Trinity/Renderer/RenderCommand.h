#pragma once

#include "Trinity/Renderer/Renderer.h"

#include <string>
#include <cstdint>

namespace Trinity
{
	class Window;

	class RenderCommand
	{
	public:
		static void Initialize(Window& window, RendererAPI api = RendererAPI::VULKAN);
		static void Shutdown();

		static void Resize(uint32_t width, uint32_t height);

		static void BeginFrame();
		static void EndFrame();

		//static void DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		static Renderer& GetRenderer();
		static std::string ApiToString(RendererAPI api);
	};
}