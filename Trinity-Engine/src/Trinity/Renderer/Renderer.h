#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Trinity
{
	class Window;

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

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color)
		{
			DrawMesh(primitive, position, color, glm::mat4(1.0f));
		}

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection) = 0;

		virtual void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
		{
			DrawMesh(primitive, position, color, projection * view);
		}


	protected:
		explicit Renderer(RendererAPI api) : m_CurrentAPI(api) {}

	protected:
		RendererAPI m_CurrentAPI = RendererAPI::NONE;
	};
}