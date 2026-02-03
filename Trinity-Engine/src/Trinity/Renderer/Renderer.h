#pragma once

#include "Trinity/Utilities/Utilities.h"

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
		inline Renderer() : m_CurrentAPI(RendererAPI::VULKAN)
		{
			switch (m_CurrentAPI)
			{
				case RendererAPI::NONE:
					TR_CORE_CRITICAL("No Renderer API was selected");
					break;

				case RendererAPI::VULKAN:
					break;

				case RendererAPI::MOLTENVK:
					break;

				case RendererAPI::DIRECTX:
					break;
			}
		}
		virtual ~Renderer() = default;

		virtual void SetWindow(Window& window) = 0;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

	protected:
		RendererAPI m_CurrentAPI;
	};
}