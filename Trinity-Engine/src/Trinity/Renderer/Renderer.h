#pragma once

#include "Trinity/Renderer/Buffer.h"
#include "Trinity/Renderer/Texture.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Trinity
{
	class Window;
	class ImGuiLayer;

	namespace Geometry
	{
		enum class PrimitiveType : uint8_t;
	}

	enum class RendererAPI
	{
		NONE = 0,
		VULKAN,
		MOLTENVK,
		DIRECTX,
	};

	struct SceneLightData;

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		RendererAPI GetAPI() const { return m_CurrentAPI; }

		virtual void SetWindow(Window& window) = 0;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void RenderImGui(ImGuiLayer& imGuiLayer) = 0;
		virtual void SetSceneViewportSize(uint32_t width, uint32_t height) = 0;
		virtual void* GetSceneViewportHandle() const = 0;

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection) = 0;
		virtual void DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection) = 0;

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
		{
			DrawMesh(primitive, model, color, projection * view);
		}

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection)
		{
			DrawMesh(primitive, glm::translate(glm::mat4(1.0f), position), color, viewProjection);
		}

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
		{
			DrawMesh(primitive, glm::translate(glm::mat4(1.0f), position), color, projection * view);
		}

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color)
		{
			DrawMesh(primitive, glm::translate(glm::mat4(1.0f), position), color, glm::mat4(1.0f));
		}

		virtual void BeginShadowPass(const glm::mat4& lightSpaceMatrix) {}
		virtual void EndShadowPass() {}
		virtual void DrawMeshShadow(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& lightSpaceMVP) {}

		virtual void BeginGeometryPass() {}
		virtual void EndGeometryPass() {}
		virtual void DrawMeshDeferred(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::mat4& viewProjection, Texture2D* albedoTexture) {}

		virtual void BeginLightingPass() {}
		virtual void EndLightingPass() {}
		virtual void UploadLights(const void* lightData, uint32_t byteSize) {}
		virtual void DrawLightingQuad(const glm::mat4& invViewProjection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar) {}

	protected:
		explicit Renderer(RendererAPI api) : m_CurrentAPI(api) {}

	protected:
		RendererAPI m_CurrentAPI = RendererAPI::NONE;
	};
}