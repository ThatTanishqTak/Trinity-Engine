#include "ClientLayer.h"

#include "Client/Camera/EditorCamera.h"
#include "Engine/Utilities/Utilities.h"
#include "Engine/Input/Input.h"
#include "Engine/Application/Application.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/Renderer.h"

ClientLayer::ClientLayer() : Engine::Layer("ClientLayer")
{

}

void ClientLayer::OnInitialize()
{
    Engine::Render::RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));

    m_EditorCamera = std::make_unique<EditorCamera>();
    Engine::Render::Renderer::SetActiveCamera(m_EditorCamera.get());
}

ClientLayer::~ClientLayer() = default;

void ClientLayer::OnInitialize()
{
    Engine::Render::RenderCommand::SetClearColor(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
}

void ClientLayer::OnShutdown()
{

}

void ClientLayer::OnUpdate(float deltaTime)
{
    if (m_EditorCamera)
    {
        m_EditorCamera->OnUpdate(deltaTime);
    }
}

void ClientLayer::OnRender()
{
    Engine::Render::RenderCommand::Clear();
    Engine::Render::RenderCommand::DrawCube();
}

void ClientLayer::OnImGuiRender()
{

}

void ClientLayer::OnEvent(Engine::Event& e)
{
    if (Engine::Input::KeyPressed(Engine::Code::KeyCode::TR_KEY_ESCAPE))
    {
        // Close Client
        Engine::Application::Close();
    }
}