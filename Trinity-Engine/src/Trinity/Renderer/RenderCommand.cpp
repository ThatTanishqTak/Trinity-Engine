#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Renderer/RendererFactory.h"

#include "Trinity/Utilities/Log.h"

#include <memory>

namespace Trinity
{
	static std::unique_ptr<Renderer> s_Renderer = nullptr;

	void RenderCommand::Initialize(Window& window, RendererAPI api)
	{
		if (s_Renderer != nullptr)
		{
			TR_CORE_WARN("RenderCommand::Initialize called while renderer already exists. Reinitializing");
			s_Renderer->Shutdown();
			s_Renderer.reset();
		}

		s_Renderer = RendererFactory::Create(api);
		if (s_Renderer == nullptr)
		{
			TR_CORE_CRITICAL("RenderCommand::Initialize failed: RendererFactory returned nullptr");
		}
		TR_CORE_TRACE("Selected API: {}", ApiToString(api));

		s_Renderer->SetWindow(window);

		TR_CORE_INFO("------- INITIALIZING RENDERER -------");

		s_Renderer->Initialize();
		
		TR_CORE_INFO("------- RENDERER INITIALIZED -------");
	}

	void RenderCommand::Shutdown()
	{
		if (s_Renderer == nullptr)
		{
			return;
		}

		TR_CORE_INFO("------- SHUTTING DOWN RENDERER -------");

		s_Renderer->Shutdown();
		s_Renderer.reset();

		TR_CORE_INFO("------- RENDERER SHUTDOWN COMPLETE -------");
	}

	void RenderCommand::Resize(uint32_t width, uint32_t height)
	{
		if (s_Renderer == nullptr)
		{
			TR_CORE_WARN("RenderCommand::Resize called before renderer initialization");

			return;
		}

		s_Renderer->Resize(width, height);
	}

	void RenderCommand::BeginFrame()
	{
		if (s_Renderer == nullptr)
		{
			return;
		}

		s_Renderer->BeginFrame();
	}

	void RenderCommand::EndFrame()
	{
		if (s_Renderer == nullptr)
		{
			return;
		}

		s_Renderer->EndFrame();
	}

	Renderer& RenderCommand::GetRenderer()
	{
		if (s_Renderer == nullptr)
		{
			TR_CORE_CRITICAL("RenderCommand::GetRenderer called before renderer initialization");
		}

		return *s_Renderer;
	}

	std::string RenderCommand::ApiToString(RendererAPI api)
	{
		switch (api)
		{
			case RendererAPI::VULKAN:
				return "Vulkan";
			case RendererAPI::MOLTENVK:
				return "MoltenVK";
			case RendererAPI::DIRECTX:
				return "DirectX";
			default:
				return "Unknown API";
		}
	}
}