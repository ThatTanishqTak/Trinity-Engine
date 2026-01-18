#pragma once

#include "Engine/Layer/Layer.h"

class ClientLayer : public Engine::Layer
{
public:
    ClientLayer();
    ~ClientLayer() override;

    void OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(float deltaTime) override;
    void OnRender() override;
};