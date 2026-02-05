#pragma once

#include "Trinity/Geometry/Vertex.h"
#include "Trinity/Renderer/MeshHandle.h"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <vector>

namespace Trinity
{
	class Window;

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

		// Phase 1: Scene submission (default no-op for placeholder backends)
		virtual void BeginScene(const glm::mat4& a_ViewProjection) { (void)a_ViewProjection; }
		virtual void SubmitMesh(MeshHandle a_Mesh, const glm::mat4& a_Transform) { (void)a_Mesh; (void)a_Transform; }
		virtual void EndScene() {}

		// Phase 1: Renderer-owned meshes (default invalid for placeholder backends)
		virtual MeshHandle CreateMesh(const std::vector<Geometry::Vertex>& vertices, const std::vector<uint32_t>& indices)
		{
			(void)vertices;
			(void)indices;

			return InvalidMeshHandle;
		}

		virtual void DestroyMesh(MeshHandle handle) { (void)handle; }

	protected:
		explicit Renderer(RendererAPI api) : m_CurrentAPI(api) {}

	protected:
		RendererAPI m_CurrentAPI = RendererAPI::NONE;
	};
}