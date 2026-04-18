#include "Trinity/Renderer/Renderer.h"

namespace Trinity
{
	std::unique_ptr<RendererAPI> Renderer::s_API = nullptr;

	void Renderer::Initialize(Window& window, const RendererSpecification& specification)
	{
		RendererAPISpecification l_APISpecification;
		l_APISpecification.MaxFramesInFlight = specification.MaxFramesInFlight;
		l_APISpecification.EnableValidation = specification.EnableValidation;

		s_API = RendererAPI::Create(specification.Backend);
		s_API->Initialize(window, l_APISpecification);
	}

	void Renderer::Shutdown()
	{
		if (!s_API)
		{
			return;
		}

		s_API->Shutdown();
		s_API.reset();
	}

	bool Renderer::BeginFrame()
	{
		if (!s_API)
		{
			return false;
		}

		return s_API->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		if (!s_API)
		{
			return;
		}

		s_API->EndFrame();
	}

	void Renderer::Present()
	{
		if (!s_API)
		{
			return;
		}

		s_API->Present();
	}

	void Renderer::WaitIdle()
	{
		if (!s_API)
		{
			return;
		}

		s_API->WaitIdle();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		if (!s_API)
		{
			return;
		}

		s_API->OnWindowResize(width, height);
	}

	std::shared_ptr<Texture> Renderer::CreateTextureFromData(const void* data, uint32_t width, uint32_t height)
	{
		if (!s_API)
		{
			return nullptr;
		}

		return s_API->CreateTextureFromData(data, width, height);
	}

	std::shared_ptr<Texture> Renderer::LoadTextureFromFile(const std::string& path)
	{
		if (!s_API)
		{
			return nullptr;
		}

		return s_API->LoadTextureFromFile(path);
	}

	RendererBackend Renderer::GetBackend()
	{
		if (!s_API)
		{
			return RendererBackend::None;
		}

		return s_API->GetBackend();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		if (!s_API)
		{
			return 0;
		}

		return s_API->GetCurrentFrameIndex();
	}

	uint32_t Renderer::GetMaxFramesInFlight()
	{
		if (!s_API)
		{
			return 0;
		}

		return s_API->GetMaxFramesInFlight();
	}
}