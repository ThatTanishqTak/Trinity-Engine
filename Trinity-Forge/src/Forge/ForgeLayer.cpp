#include "ForgeLayer.h"

#include "Trinity/Utilities/Utilities.h"
#include "Trinity/Input/Input.h"

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
    //for (int i = 0; i < 20; i++)
    //{
    //    Trinity::RenderCommand::DrawCube({ 1.0f * i, 1.0f * i, 1.0f * i }, { 0.0f, 0.0f, 0.0f });
    //}
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