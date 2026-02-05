#pragma once

#include "Trinity/Renderer/Renderer.h"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class Window;

	class RenderCommand
	{
	public:
		static void Initialize(Window& window, RendererAPI api = RendererAPI::VULKAN);
		static void Shutdown();

		static void Resize(uint32_t width, uint32_t height);

		static void BeginFrame();
		static void EndFrame();

		// Phase 1: scene API
		static void BeginScene(const glm::mat4& viewProjection);
		static void SubmitMesh(MeshHandle mesh, const glm::mat4& transform);
		static void EndScene();

		//static void DrawCube(glm::vec3 pos, glm::vec3 size, glm::vec4 tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		static Renderer& GetRenderer();
	};
}