#include "ClientLayer.h"

#include "Engine/Core/Utilities.h"
#include "Engine/Application/Application.h"
#include "Engine/Events/Input/Input.h"

#include "Engine/Events/Event.h"
#include "Engine/Events/EventDispatcher.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"

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

}

void ClientLayer::OnEvent(Engine::Event& e)
{
    Engine::EventDispatcher dispatcher(e);

    dispatcher.Dispatch<Engine::KeyPressedEvent>([](Engine::KeyPressedEvent& ev)
        {
            return false;
        });

    dispatcher.Dispatch<Engine::MouseScrolledEvent>([](Engine::MouseScrolledEvent& ev)
        {
            return false;
        });
}