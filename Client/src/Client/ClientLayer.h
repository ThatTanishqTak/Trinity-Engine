#pragma once

#include "Engine/Layer/Layer.h"

namespace Engine
{
    class Event;
}

class ClientLayer : public Engine::Layer
{
public:
    ClientLayer();
    ~ClientLayer() override;

    void OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    void OnImGuiRender() override;

    void OnEvent(Engine::Event& e) override;
};