#include "Trinity/Renderer/SceneRenderer.h"

#include "Trinity/Renderer/Passes/DeferredLightingPass.h"
#include "Trinity/Renderer/Passes/GeometryPass.h"
#include "Trinity/Renderer/Passes/SceneRenderPassContext.h"
#include "Trinity/Renderer/RenderGraph/RenderGraph.h"
#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Utilities/Log.h"

#include <memory>

namespace Trinity
{
    struct SceneRenderer::Implementation
    {
        SceneRendererStats Stats;
        bool SceneActive = false;

        std::unique_ptr<RenderGraph> Graph;

        SceneRenderGraphResources Resources;
        SceneRenderPassContext PassContext;

        GeometryPass Geometry;
        DeferredLightingPass DeferredLighting;
    };

    SceneRenderer::SceneRenderer() = default;

    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Initialize(uint32_t width, uint32_t height)
    {
        TR_CORE_INFO("INITIALIZING SCENE RENDERER");

        m_Width = width;
        m_Height = height;

        m_Implementation = std::make_unique<Implementation>();

        m_Implementation->Graph = Renderer::GetAPI().CreateRenderGraph();

        if (!m_Implementation->Graph)
        {
            TR_CORE_ERROR("SceneRenderer failed to create RenderGraph");
            return;
        }

        m_Implementation->Graph->OnResize(width, height);

        m_Implementation->PassContext.Width = width;
        m_Implementation->PassContext.Height = height;
        m_Implementation->PassContext.Stats = &m_Implementation->Stats;
        m_Implementation->PassContext.ActiveCamera = &m_Camera;
        m_Implementation->PassContext.SceneData = &m_SceneData;
        m_Implementation->PassContext.DrawList = &m_DrawList;

        m_Implementation->Geometry.Initialize();
        m_Implementation->DeferredLighting.Initialize();

        m_Implementation->Geometry.DeclareResources(*m_Implementation->Graph, m_Implementation->Resources);
        m_Implementation->DeferredLighting.DeclareResources(*m_Implementation->Graph, m_Implementation->Resources);

        m_Implementation->Geometry.AddToGraph(*m_Implementation->Graph, m_Implementation->Resources, m_Implementation->PassContext);
        m_Implementation->DeferredLighting.AddToGraph(*m_Implementation->Graph, m_Implementation->Resources, m_Implementation->PassContext);

        m_Implementation->Graph->AddPass("ViewportOutput").SetType(RenderGraphPassType::Graphics).SetCullable(false).SetDebugColor(0.05f, 0.65f, 0.25f, 1.0f).Read(m_Implementation->Resources.LitOutput, RenderGraphAccess::ShaderSampledRead).SetExecuteCallback([](RenderGraphContext& context)
        {
            (void)context;
        });

        m_Implementation->Graph->MarkOutput(m_Implementation->Resources.LitOutput);

        TR_CORE_INFO("SCENE RENDERER INITIALIZED");
    }

    void SceneRenderer::Shutdown()
    {
        TR_CORE_INFO("SHUTTING DOWN SCENE RENDERER");

        if (m_Implementation)
        {
            m_Implementation->DeferredLighting.Shutdown();
            m_Implementation->Geometry.Shutdown();
            m_Implementation.reset();
        }

        TR_CORE_INFO("SCENE RENDERER SHUTDOWN COMPLETE");
    }

    void SceneRenderer::SetSettings(const RenderPipelineSettings& settings)
    {
        m_Settings = settings;
    }

    void SceneRenderer::BeginScene(const Camera& camera, const SceneRenderData& sceneData)
    {
        m_Camera = camera;
        m_SceneData = sceneData;

        m_DrawList.clear();

        if (!m_Implementation)
        {
            return;
        }

        m_Implementation->Stats = {};
        m_Implementation->SceneActive = true;

        m_Implementation->PassContext.Width = m_Width;
        m_Implementation->PassContext.Height = m_Height;
        m_Implementation->PassContext.Stats = &m_Implementation->Stats;
        m_Implementation->PassContext.ActiveCamera = &m_Camera;
        m_Implementation->PassContext.SceneData = &m_SceneData;
        m_Implementation->PassContext.DrawList = &m_DrawList;
    }

    void SceneRenderer::SubmitMesh(const MeshDrawCommand& command)
    {
        m_DrawList.push_back(command);
    }

    void SceneRenderer::EndScene()
    {
        if (!m_Implementation)
        {
            return;
        }

        m_Implementation->SceneActive = false;
    }

    void SceneRenderer::Render()
    {
        if (!m_Implementation || !m_Implementation->Graph || !m_Implementation->Geometry.IsReady() || !m_Implementation->DeferredLighting.IsReady())
        {
            return;
        }

        m_Implementation->Graph->Execute();
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        if (m_Width == width && m_Height == height)
        {
            return;
        }

        Renderer::WaitIdle();

        m_Width = width;
        m_Height = height;

        if (!m_Implementation)
        {
            return;
        }

        m_Implementation->PassContext.Width = width;
        m_Implementation->PassContext.Height = height;

        if (m_Implementation->Graph)
        {
            m_Implementation->Graph->OnResize(width, height);
        }
    }

    std::shared_ptr<Texture> SceneRenderer::GetFinalOutput() const
    {
        if (!m_Implementation || !m_Implementation->Graph)
        {
            return nullptr;
        }

        return m_Implementation->Graph->GetTexture(m_Implementation->Resources.LitOutput);
    }

    const SceneRendererStats& SceneRenderer::GetStats() const
    {
        static SceneRendererStats s_EmptyStats{};

        if (!m_Implementation)
        {
            return s_EmptyStats;
        }

        return m_Implementation->Stats;
    }

    std::string SceneRenderer::DumpRenderGraphText() const
    {
        if (!m_Implementation || !m_Implementation->Graph)
        {
            return {};
        }

        return m_Implementation->Graph->DumpText();
    }

    std::string SceneRenderer::DumpRenderGraphDot() const
    {
        if (!m_Implementation || !m_Implementation->Graph)
        {
            return {};
        }

        return m_Implementation->Graph->DumpDot();
    }
}