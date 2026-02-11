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

	void DirectXRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color)
	{
		DrawMesh(primitive, position, color, glm::mat4(1.0f));
	}

	void DirectXRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		(void)primitive;
		(void)position;
		(void)color;
		(void)viewProjection;
	}

	void DirectXRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		DrawMesh(primitive, position, color, projection * view);
	}

	void DirectXRenderer::BeginFrame()
	{

	}

	void DirectXRenderer::EndFrame()
	{

	}
}