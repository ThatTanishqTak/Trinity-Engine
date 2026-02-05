#include "ForgeLayer.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Trinity/Platform/Window.h"
#include "Trinity/Application/Application.h"
#include "Trinity/Geometry/Vertex.h"
#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Utilities/Utilities.h"
#include "Trinity/Input/Input.h"

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer") {}
ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{
    std::vector<Trinity::Geometry::Vertex> l_Vertices =
    {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
    };

    std::vector<uint32_t> l_Indices = { 0, 1, 2 };

    m_TriangleMesh = Trinity::RenderCommand::CreateMesh(l_Vertices, l_Indices);

    if (m_TriangleMesh == Trinity::InvalidMeshHandle)
    {
        TR_CORE_ERROR("ForgeLayer failed to create test mesh");
    }
}

void ForgeLayer::OnShutdown()
{
    if (m_TriangleMesh != Trinity::InvalidMeshHandle)
    {
        Trinity::RenderCommand::DestroyMesh(m_TriangleMesh);
        m_TriangleMesh = Trinity::InvalidMeshHandle;
    }
}

void ForgeLayer::OnUpdate(float deltaTime)
{
    (void)deltaTime;
}

void ForgeLayer::OnRender()
{
    if (m_TriangleMesh == Trinity::InvalidMeshHandle)
    {
        return;
    }

    Trinity::Window& l_Window = Trinity::Application::Get().GetWindow();
    const uint32_t l_Width = l_Window.GetWidth();
    const uint32_t l_Height = l_Window.GetHeight();
    const float l_Aspect = (l_Height == 0) ? 1.0f : static_cast<float>(l_Width) / static_cast<float>(l_Height);

    glm::mat4 l_Proj = glm::perspective(glm::radians(45.0f), l_Aspect, 0.1f, 100.0f);
    l_Proj[1][1] *= -1.0f;

    glm::mat4 l_View = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 l_ViewProj = l_Proj * l_View;

    Trinity::RenderCommand::BeginScene(l_ViewProj);

    glm::mat4 l_ModelA = glm::translate(glm::mat4(1.0f), glm::vec3(-0.8f, 0.0f, 0.0f));
    glm::mat4 l_ModelB = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 l_ModelC = glm::translate(glm::mat4(1.0f), glm::vec3(0.8f, 0.0f, 0.0f));

    Trinity::RenderCommand::SubmitMesh(m_TriangleMesh, l_ModelA);
    Trinity::RenderCommand::SubmitMesh(m_TriangleMesh, l_ModelB);
    Trinity::RenderCommand::SubmitMesh(m_TriangleMesh, l_ModelC);

    Trinity::RenderCommand::EndScene();
}

void ForgeLayer::OnImGuiRender() {}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;

    if (Trinity::Input::KeyPressed(Trinity::Code::KeyCode::TR_KEY_ESCAPE))
    {
        Trinity::Application::Close();
    }
}