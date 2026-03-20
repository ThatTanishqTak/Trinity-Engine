#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Renderer/RendererFactory.h"

#include "Trinity/Utilities/Log.h"

#include <memory>

namespace Trinity
{
	static std::unique_ptr<Renderer> s_Renderer = nullptr;

	void RenderCommand::Initialize(Window& window, RendererAPI api)
	{
		if (s_Renderer != nullptr)
		{
			TR_CORE_WARN("RenderCommand::Initialize called while renderer already exists. Reinitializing.");

			s_Renderer->Shutdown();
			s_Renderer.reset();
		}

		s_Renderer = RendererFactory::Create(api);
		if (!s_Renderer)
		{
			TR_CORE_CRITICAL("RenderCommand::Initialize: RendererFactory returned nullptr");

			std::abort();
		}

		TR_CORE_TRACE("Selected API: {}", ApiToString(api));

		s_Renderer->SetWindow(window);
		s_Renderer->Initialize();
	}

	void RenderCommand::Shutdown()
	{
		if (!s_Renderer)
		{
			return;
		}

		s_Renderer->Shutdown();
		s_Renderer.reset();
	}

	void RenderCommand::Resize(uint32_t width, uint32_t height)
	{
		if (s_Renderer)
		{
			s_Renderer->Resize(width, height);
		}
	}

	void RenderCommand::BeginFrame()
	{
		if (s_Renderer)
		{
			s_Renderer->BeginFrame();
		}
	}

	void RenderCommand::EndFrame()
	{
		if (s_Renderer)
		{
			s_Renderer->EndFrame();
		}
	}

	void RenderCommand::RenderImGui(ImGuiLayer& imGuiLayer)
	{
		if (s_Renderer)
		{
			s_Renderer->RenderImGui(imGuiLayer);
		}
	}

	void RenderCommand::SetSceneViewportSize(uint32_t width, uint32_t height)
	{
		if (s_Renderer)
		{
			s_Renderer->SetSceneViewportSize(width, height);
		}
	}

	void* RenderCommand::GetSceneViewportHandle()
	{
		if (!s_Renderer)
		{
			return nullptr;
		}

		return s_Renderer->GetSceneViewportHandle();
	}

	Renderer& RenderCommand::GetRenderer()
	{
		if (!s_Renderer)
		{
			TR_CORE_CRITICAL("RenderCommand::GetRenderer called before Initialize");

			std::abort();
		}

		return *s_Renderer;
	}

	void RenderCommand::DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawMesh(primitive, model, color, viewProjection);
		}
	}

	void RenderCommand::DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawMesh(primitive, model, color, view, projection);
		}
	}

	void RenderCommand::DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawMesh(vertexBuffer, indexBuffer, indexCount, model, color, viewProjection);
		}
	}

	void RenderCommand::DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawMesh(vertexBuffer, indexBuffer, indexCount, model, color, projection * view);
		}
	}

	void RenderCommand::BeginShadowPass(const glm::mat4& lightSpaceMatrix)
	{
		if (s_Renderer)
		{
			s_Renderer->BeginShadowPass(lightSpaceMatrix);
		}
	}

	void RenderCommand::EndShadowPass()
	{
		if (s_Renderer)
		{
			s_Renderer->EndShadowPass();
		}
	}

	void RenderCommand::DrawMeshShadow(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& lightSpaceMVP)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawMeshShadow(vertexBuffer, indexBuffer, indexCount, lightSpaceMVP);
		}
	}

	void RenderCommand::BeginGeometryPass()
	{
		if (s_Renderer)
		{
			s_Renderer->BeginGeometryPass();
		}
	}

	void RenderCommand::EndGeometryPass()
	{
		if (s_Renderer)
		{
			s_Renderer->EndGeometryPass();
		}
	}

	void RenderCommand::DrawMeshDeferred(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::mat4& viewProjection, const glm::vec4& color, Texture2D* albedoTexture)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawMeshDeferred(vertexBuffer, indexBuffer, indexCount, model, viewProjection, color, albedoTexture);
		}
	}

	void RenderCommand::BeginLightingPass()
	{
		if (s_Renderer)
		{
			s_Renderer->BeginLightingPass();
		}
	}

	void RenderCommand::EndLightingPass()
	{
		if (s_Renderer)
		{
			s_Renderer->EndLightingPass();
		}
	}

	void RenderCommand::UploadLights(const void* lightData, uint32_t byteSize)
	{
		if (s_Renderer)
		{
			s_Renderer->UploadLights(lightData, byteSize);
		}
	}

	void RenderCommand::DrawLightingQuad(const glm::mat4& invViewProjection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar)
	{
		if (s_Renderer)
		{
			s_Renderer->DrawLightingQuad(invViewProjection, cameraPosition, cameraNear, cameraFar);
		}
	}

	void RenderCommand::BeginPostProcessPass()
	{
		if (s_Renderer)
		{
			s_Renderer->BeginPostProcessPass();
		}
	}

	void RenderCommand::EndPostProcessPass()
	{
		if (s_Renderer)
		{
			s_Renderer->EndPostProcessPass();
		}
	}

	void RenderCommand::DrawPostProcessQuad()
	{
		if (s_Renderer)
		{
			s_Renderer->DrawPostProcessQuad();
		}
	}

	std::string RenderCommand::ApiToString(RendererAPI api)
	{
		switch (api)
		{
			case RendererAPI::VULKAN:
				return "Vulkan";
			case RendererAPI::MOLTENVK:
				return "MoltenVK";
			case RendererAPI::DIRECTX:
				return "DirectX 12";
			default:
				return "None";
		}
	}
}