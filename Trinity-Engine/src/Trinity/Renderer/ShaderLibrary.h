#pragma once

#include "Trinity/Renderer/Shader.h"

#include <string>
#include <unordered_map>

namespace Trinity
{
	class ShaderLibrary
	{
	public:
		const Shader* Load(const std::string& name, const std::string& spvPath, ShaderStage stage);
		const Shader* Compile(const std::string& name, const std::string& sourcePath, ShaderStage stage);
		const Shader* Get(const std::string& name) const;

		bool Has(const std::string& name) const;
		void Remove(const std::string& name);
		void Clear();
		bool Reload(const std::string& name);
		void HotReload();

		const std::vector<uint32_t>* GetSpirV(const std::string& name) const;

	private:
		static ShaderReflectionData ReflectSpirV(const std::vector<uint32_t>& spirV);
		static std::vector<uint32_t> CompileGlsl(const std::string& sourcePath, ShaderStage stage);

	private:
		std::unordered_map<std::string, Shader> m_Shaders;
	};
}