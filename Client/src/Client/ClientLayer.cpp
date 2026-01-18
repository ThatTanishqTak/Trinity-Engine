#include "ClientLayer.h"
#include <iostream>

#include "Engine/Core/Utilities.h"

ClientLayer::ClientLayer() : Engine::Layer("ClientLayer")
{

}

ClientLayer::~ClientLayer() = default;

void ClientLayer::OnInitialize()
{
    TR_TRACE("[{}] Initialized", GetName());
}

void ClientLayer::OnShutdown()
{
    TR_TRACE("[{}] Shutdown", GetName());
}

void ClientLayer::OnUpdate(float deltaTime)
{
    TR_TRACE("[{}] UPDATE: deltaTime = {}", GetName(), deltaTime);
}

void ClientLayer::OnRender()
{
    TR_TRACE("[{}] RENDER", GetName());
}