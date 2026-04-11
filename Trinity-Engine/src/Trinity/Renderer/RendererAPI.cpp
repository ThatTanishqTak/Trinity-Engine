#include "Trinity/Renderer/RendererAPI.h"

#include "Trinity/Utilities/Log.h"

#include <cstdlib>

namespace Trinity
{
	std::unique_ptr<RendererAPI> RendererAPI::Create(RendererBackend backend)
	{
		TR_CORE_CRITICAL("Requested renderer backend is not compiled into this build.");
		std::abort();

		return nullptr;
	}
}