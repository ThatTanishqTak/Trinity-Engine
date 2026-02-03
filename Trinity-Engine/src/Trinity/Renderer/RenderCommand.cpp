#include "Trinity/Renderer/RenderCommand.h"

#include "Trinity/Renderer/RendererFactory.h"
#include "Trinity/Utilities/Utilities.h"

#include <memory>
#include <cstdlib>

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

			std::abort();
		}

		s_Renderer->SetWindow(window);
		s_Renderer->Initialize();
	}

	void RenderCommand::Shutdown()
	{
		if (s_Renderer == nullptr)
		{
			return;
		}

		s_Renderer->Shutdown();
		s_Renderer.reset();
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

			std::abort();
		}

		return *s_Renderer;
	}
}