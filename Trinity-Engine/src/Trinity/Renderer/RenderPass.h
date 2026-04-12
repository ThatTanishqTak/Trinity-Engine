#pragma once

#include "Trinity/Renderer/Resources/Framebuffer.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Trinity
{
	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;

		virtual void Begin(uint32_t width, uint32_t height) = 0;
		virtual void End() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual std::shared_ptr<Framebuffer> GetFramebuffer() const = 0;

		const std::string& GetName() const { return m_String; }

	protected:
		std::string m_String;
	};
}