#pragma once
#include <Renderer/Shared/GfxTypes.h>


namespace YAPT
{
	class ShaderModuleVk
	{
	public:
		ShaderModuleVk(const char* filepath, ShaderModuleType moduleType, const char* entryPoint, const ShaderModuleDefine* defines, size_t defineCount);
		~ShaderModuleVk();

	};
}