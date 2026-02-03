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

}

void ForgeLayer::OnRender()
{

}

void ForgeLayer::OnImGuiRender()
{

}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
    if (Trinity::Input::KeyPressed(Trinity::Code::KeyCode::TR_KEY_ESCAPE))
    {
        // Close Forge
        Trinity::Application::Close();
    }
}