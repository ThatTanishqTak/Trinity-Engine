#pragma once

#include "Trinity/Layer/Layer.h"
#include "Trinity/Renderer/MeshHandle.h"

namespace Trinity
{
    class Event;
}

class ForgeLayer : public Trinity::Layer
{
public:
    ForgeLayer();
    ~ForgeLayer() override;

    void OnInitialize() override;
    void OnShutdown() override;

    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    void OnImGuiRender() override;

    void OnEvent(Trinity::Event& e) override;

private:
    Trinity::MeshHandle m_TriangleMesh = Trinity::InvalidMeshHandle;
};