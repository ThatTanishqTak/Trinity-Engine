#include "ClientLayer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Application/Application.h"

#include "Engine/Renderer/RenderCommand.h"

#include "Engine/Input/Input.h"

ClientLayer::ClientLayer() : Engine::Layer("ClientLayer")
{

}

ClientLayer::~ClientLayer() = default;

void ClientLayer::OnInitialize()
{

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
    Engine::Render::DrawCube(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void ClientLayer::OnImGuiRender()
{

}

void ClientLayer::OnEvent(Engine::Event& e)
{

}