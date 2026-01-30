#pragma once

#include "Engine/Layer/Layer.h"
#include "Client/Camera/EditorCamera.h"

#include <memory>

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

private:
    std::unique_ptr<EditorCamera> m_EditorCamera;
};