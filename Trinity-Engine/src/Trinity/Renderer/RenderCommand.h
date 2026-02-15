#pragma once

#include "Trinity/Renderer/Renderer.h"

#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace Trinity
{
	class Window;
	class ImGuiLayer;

	class RenderCommand
	{
	public:
		static void Initialize(Window& window, RendererAPI api = RendererAPI::VULKAN);
		static void Shutdown();

		static void Resize(uint32_t width, uint32_t height);

		static void BeginFrame();
		static void EndFrame();
		static void RenderImGui(ImGuiLayer& imGuiLayer);
		static void SetSceneViewportSize(uint32_t width, uint32_t height);
		static void* GetSceneViewportHandle();

		static void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color);
		static void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection);
		static void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection);

		static Renderer& GetRenderer();
		static std::string ApiToString(RendererAPI api);
	};
}