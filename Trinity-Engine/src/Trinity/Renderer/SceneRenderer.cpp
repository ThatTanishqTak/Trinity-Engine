#include "Trinity/Renderer/SceneRenderer.h"

#include <memory>

namespace Trinity
{
	struct SceneRenderer::Impl
	{
		SceneRendererStats Stats;
		bool SceneActive = false;
	};

	void SceneRenderer::Initialize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
		m_Implementation = std::make_unique<Impl>();
	}

	void SceneRenderer::Shutdown()
	{
		m_Implementation.reset();
	}

	void SceneRenderer::BeginScene(const Camera& camera, const SceneRenderData& sceneData)
	{
		m_Camera = camera;
		m_SceneData = sceneData;
		m_DrawList.clear();
		m_Implementation->Stats = {};
		m_Implementation->SceneActive = true;
	}

	void SceneRenderer::SubmitMesh(const MeshDrawCommand& command)
	{
		m_DrawList.push_back(command);
	}

	void SceneRenderer::EndScene()
	{
		m_Implementation->SceneActive = false;
	}

	void SceneRenderer::Render()
	{
		m_Implementation->Stats.DrawCalls = static_cast<uint32_t>(m_DrawList.size());
	}

	void SceneRenderer::OnResize(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
	}

	std::shared_ptr<Texture> SceneRenderer::GetFinalOutput() const
	{
		return nullptr;
	}

	const SceneRendererStats& SceneRenderer::GetStats() const
	{
		return m_Implementation->Stats;
	}
}