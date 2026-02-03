#pragma once

#include "Trinity/Renderer/Renderer.h"

namespace Trinity
{
	// WILL BE WORKED ON IN THE FUTURE
	class DirectXRenderer : public Renderer
	{
	public:
		DirectXRenderer();
		~DirectXRenderer();

		void SetWindow(Window& window) override;

		void Initialize() override;
		void Shutdown() override;

		void Resize(uint32_t width, uint32_t height) override;

		void BeginFrame() override;
		void EndFrame() override;

	private:

	};
}