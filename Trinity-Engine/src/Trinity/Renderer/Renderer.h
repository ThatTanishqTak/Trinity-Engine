#pragma once

#include <cstdint>

namespace Trinity
{
	class Window;

	enum class RendererAPI
	{
		NONE = 0,

		VULKAN,
		MOLTENVK,
		DIRECTX,
	};

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		RendererAPI GetAPI() const { return m_CurrentAPI; }

		virtual void SetWindow(Window& window) = 0;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

	protected:
		explicit Renderer(RendererAPI api) : m_CurrentAPI(api)
		{

		}

	protected:
		RendererAPI m_CurrentAPI = RendererAPI::NONE;
	};
}