#include "ClientLayer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Input/Input.h"
#include "Engine/Application/Application.h"
#include "Engine/Renderer/RenderCommand.h"

ClientLayer::ClientLayer() : Engine::Layer("ClientLayer")
{

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
    (void)deltaTime;
}

void ClientLayer::OnRender()
{
    Engine::Render::RenderCommand::Clear();
    Engine::Render::RenderCommand::DrawTriangle();
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