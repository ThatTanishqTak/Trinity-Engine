#include "Trinity/Renderer/MoltenVK/MoltenVKRenderer.h"

#include "Trinity/Utilities/Log.h"

// WILL BE WORKED ON IN THE FUTURE

namespace Trinity
{
	MoltenVKRenderer::MoltenVKRenderer() : Renderer(RendererAPI::MOLTENVK)
	{

	}

	MoltenVKRenderer::~MoltenVKRenderer()
	{

	}

	void MoltenVKRenderer::SetWindow(Window& window)
	{
		(void)window;
	}

	void MoltenVKRenderer::Initialize()
	{
		TR_CORE_TRACE("Initializing MoltenVK");



		TR_CORE_TRACE("MoltenVK Initialized");
	}

	void MoltenVKRenderer::Shutdown()
	{
		TR_CORE_TRACE("Shutting Down MoltenVK");



		TR_CORE_TRACE("MoltenVK Shutdown Complete");
	}

	void MoltenVKRenderer::Resize(uint32_t width, uint32_t height)
	{
		(void)width;
		(void)height;
	}

	void MoltenVKRenderer::BeginFrame()
	{

	}

	void MoltenVKRenderer::EndFrame()
	{

	}
}