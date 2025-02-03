#pragma once

#include <Gfx/ShaderPipelineReflection.h>

namespace YAPT
{
	class ShaderPipelineReflectionDx12 : public ShaderPipelineReflection
	{
	public:
		ShaderPipelineReflectionDx12(ShaderModuleHandle* shaderModules, size_t shaderModuleCount);
		virtual ~ShaderPipelineReflectionDx12();

	private:

		void init(ShaderModuleHandle* shaderModules, size_t shaderModuleCount);

	};

}