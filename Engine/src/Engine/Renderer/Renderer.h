#pragma once

#include <cstdint>

namespace Engine
{
	class Window;

	namespace Renderer
	{
		void Initialize(Window& window);
		void Shutdown();

		void BeginFrame();
		void EndFrame();

		void OnResize(uint32_t width, uint32_t height);
	}
}