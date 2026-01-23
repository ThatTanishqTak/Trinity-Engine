#include "ClientLayer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Application/Application.h"

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

    if (Engine::Input::KeyPressed(Engine::Code::KeyCode::TR_KEY_W))
    {
        TR_TRACE("W was pressed");
    }
}

void ClientLayer::OnRender()
{

}

void ClientLayer::OnEvent(Engine::Event& e)
{

}