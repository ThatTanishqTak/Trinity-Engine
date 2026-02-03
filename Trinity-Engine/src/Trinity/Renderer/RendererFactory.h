#pragma once

#include "Trinity/Renderer/Renderer.h"

#include <memory>

namespace Trinity
{
	class RendererFactory
	{
	public:
		static std::unique_ptr<Renderer> Create(RendererAPI api);
		static std::unique_ptr<Renderer> CreateDefault();
	};
}