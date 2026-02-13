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

	void MoltenVKRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color)
	{
		DrawMesh(primitive, position, color, glm::mat4(1.0f));
	}

	void MoltenVKRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		(void)primitive;
		(void)position;
		(void)color;
		(void)viewProjection;
	}

	void MoltenVKRenderer::DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		DrawMesh(primitive, position, color, projection * view);
	}

	void MoltenVKRenderer::BeginFrame()
	{

	}

	void MoltenVKRenderer::EndFrame()
	{

	}

	void MoltenVKRenderer::RenderImGui(ImGuiLayer& imGuiLayer)
	{
		(void)imGuiLayer;
	}
}