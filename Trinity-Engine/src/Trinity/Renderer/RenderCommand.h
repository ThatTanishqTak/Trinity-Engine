#pragma once

#include "Trinity/Renderer/Renderer.h"

#include <cstdint>

namespace Trinity
{
	class Window;

	class RenderCommand
	{
	public:
		static void Initialize(Window& a_Window, RendererAPI a_API = RendererAPI::VULKAN);
		static void Shutdown();

		static void Resize(uint32_t a_Width, uint32_t a_Height);

		static void BeginFrame();
		static void EndFrame();

		static Renderer& GetRenderer();
	};
}