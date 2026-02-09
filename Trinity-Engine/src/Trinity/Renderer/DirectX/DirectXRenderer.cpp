#include "Trinity/Renderer/DirectX/DirectXRenderer.h"

#include "Trinity/Utilities/Log.h"

// WILL BE WORKED ON IN THE FUTURE

namespace Trinity
{
	DirectXRenderer::DirectXRenderer() : Renderer(RendererAPI::DIRECTX)
	{

	}

	DirectXRenderer::~DirectXRenderer()
	{

	}

	void DirectXRenderer::SetWindow(Window& window)
	{
		(void)window;
	}

	void DirectXRenderer::Initialize()
	{
		TR_CORE_TRACE("Initializing DirectX");



		TR_CORE_TRACE("DirectX Initialized");
	}

	void DirectXRenderer::Shutdown()
	{
		TR_CORE_TRACE("Shutting Down DirectX");



		TR_CORE_TRACE("DirectX Shutdown Complete");
	}

	void DirectXRenderer::Resize(uint32_t width, uint32_t height)
	{
		(void)width;
		(void)height;
	}

	void DirectXRenderer::BeginFrame()
	{

	}

	void DirectXRenderer::EndFrame()
	{

	}
}