#include "ForgeLayer.h"

#include "Trinity/Input/Input.h"
#include "Trinity/Renderer/RenderCommand.h"
#include "Trinity/Geometry/Geometry.h"
#include "Trinity/Application/Application.h"

ForgeLayer::ForgeLayer() : Trinity::Layer("ForgeLayer")
{

}

ForgeLayer::~ForgeLayer() = default;

void ForgeLayer::OnInitialize()
{

}

void ForgeLayer::OnShutdown()
{

}

void ForgeLayer::OnUpdate(float deltaTime)
{
    (void)deltaTime;
}

void ForgeLayer::OnRender()
{
    Trinity::RenderCommand::DrawMesh(Trinity::Geometry::PrimitiveType::Cube, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f });
}

void ForgeLayer::OnImGuiRender()
{

}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    (void)e;

    if (Trinity::Input::KeyPressed(Trinity::Code::KeyCode::TR_KEY_ESCAPE))
    {
        Trinity::Application::Close();
    }
}