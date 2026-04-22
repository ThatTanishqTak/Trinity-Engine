#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/ISceneRenderer.h"

#include <memory>

namespace Trinity
{
    struct SceneRenderer::Implementation
    {
        std::unique_ptr<ISceneRenderer> Backend;
        SceneRendererStats EmptyStats;
    };

    SceneRenderer::SceneRenderer() = default;
    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Initialize(uint32_t width, uint32_t height)
    {
        m_Implementation = std::make_unique<Implementation>();
        m_Implementation->Backend = CreateSceneRenderer();

        if (!m_Implementation->Backend)
        {
            return;
        }

        m_Implementation->Backend->Initialize(width, height);
    }

    void SceneRenderer::Shutdown()
    {
        if (!m_Implementation)
        {
            return;
        }

        if (m_Implementation->Backend)
        {
            m_Implementation->Backend->Shutdown();
        }

        m_Implementation.reset();
    }

    void SceneRenderer::BeginScene(const Camera& camera, const SceneRenderData& sceneData)
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            return;
        }

        m_Implementation->Backend->BeginScene(camera, sceneData);
    }

    void SceneRenderer::SubmitMesh(const MeshDrawCommand& command)
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            return;
        }

        m_Implementation->Backend->SubmitMesh(command);
    }

    void SceneRenderer::EndScene()
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            return;
        }

        m_Implementation->Backend->EndScene();
    }

    void SceneRenderer::Render()
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            return;
        }

        m_Implementation->Backend->Render();
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            return;
        }

        m_Implementation->Backend->OnResize(width, height);
    }

    std::shared_ptr<Texture> SceneRenderer::GetFinalOutput() const
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            return nullptr;
        }

        return m_Implementation->Backend->GetFinalOutput();
    }

    const SceneRendererStats& SceneRenderer::GetStats() const
    {
        if (!m_Implementation || !m_Implementation->Backend)
        {
            static SceneRendererStats s_EmptyStats{};
            return s_EmptyStats;
        }

        return m_Implementation->Backend->GetStats();
    }
}