#include "Trinity/Renderer/RendererAPI.h"

namespace Trinity
{
	std::unique_ptr<RendererAPI> RendererAPI::Create(RendererBackend backend)
	{
		return std::unique_ptr<RendererAPI>();
	}
}