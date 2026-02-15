#pragma once

#include "Trinity/Renderer/Renderer.h"

namespace Trinity
{
	// WILL BE WORKED ON IN THE FUTURE
	class DirectXRenderer : public Renderer
	{
	public:
		DirectXRenderer();
		~DirectXRenderer();

		void SetWindow(Window& window) override;

		void Initialize() override;
		void Shutdown() override;

		void Resize(uint32_t width, uint32_t height) override;

		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection) override;

		void BeginFrame() override;
		void EndFrame() override;
		void RenderImGui(ImGuiLayer& imGuiLayer) override;
		void SetSceneViewportSize(uint32_t width, uint32_t height) override;
		void* GetSceneViewportHandle() const override;

	private:

	};
}