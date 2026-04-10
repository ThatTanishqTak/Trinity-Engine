#include "ForgeLayer.h"

#include "Trinity/Events/Event.h"

#include "Trinity/Utilities/Log.h"

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

}

void ForgeLayer::OnImGuiRender()
{

}

void ForgeLayer::OnEvent(Trinity::Event& e)
{
	//TR_TRACE("{}", e.ToString());
}