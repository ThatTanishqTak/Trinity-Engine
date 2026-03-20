#pragma once

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/Buffer.h"
#include "Trinity/Renderer/Texture.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace Trinity
{
	class Window;
	class ImGuiLayer;

	namespace Geometry
	{
		enum class PrimitiveType : uint8_t;
	}

	class RenderCommand
	{
	public:
		static void Initialize(Window& window, RendererAPI api);
		static void Shutdown();

		static void Resize(uint32_t width, uint32_t height);

		static void BeginFrame();
		static void EndFrame();

		static void RenderImGui(ImGuiLayer& imGuiLayer);
		static void SetSceneViewportSize(uint32_t width, uint32_t height);
		static void* GetSceneViewportHandle();

		static Renderer& GetRenderer();

		static void DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection);
		static void DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection);
		static void DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection);
		static void DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection);

		static void BeginShadowPass(const glm::mat4& lightSpaceMatrix);
		static void EndShadowPass();
		static void DrawMeshShadow(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& lightSpaceMVP);

		static void BeginGeometryPass();
		static void EndGeometryPass();
		static void DrawMeshDeferred(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::mat4& viewProjection, const glm::vec4& color, Texture2D* albedoTexture);

		static void BeginLightingPass();
		static void EndLightingPass();
		static void UploadLights(const void* lightData, uint32_t byteSize);
		static void DrawLightingQuad(const glm::mat4& invViewProjection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar);

		static void BeginPostProcessPass();
		static void EndPostProcessPass();
		static void DrawPostProcessQuad();

	private:
		static std::string ApiToString(RendererAPI api);
	};
}