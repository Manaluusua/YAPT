#pragma once
#include <Renderer/Shared/GfxTypes.h>


namespace YAPT
{
	class ShaderModuleVk
	{
	public:
		ShaderModuleVk();
		~ShaderModuleVk();

		bool compileFromHLSL(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
	private:

		bool reflect();

		std::vector<char> m_spirv;
		
	};
}