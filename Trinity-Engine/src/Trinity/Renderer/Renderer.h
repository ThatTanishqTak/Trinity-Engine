#pragma once

namespace Trinity
{
	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
	};
}