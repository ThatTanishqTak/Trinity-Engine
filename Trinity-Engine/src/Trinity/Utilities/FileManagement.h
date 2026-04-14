#pragma once

#include <filesystem>

namespace Trinity
{
	namespace CoreUtilities
	{
		class FileManagement
		{
		public:
			static void LoadFile(std::filesystem::path path);
		};
	}
}