#pragma once

#include <Renderer/Shared/ShaderPipelineReflection.h>

namespace YAPT
{
	class ShaderPipelineReflectionVk : public ShaderPipelineReflection
	{
	public:
		ShaderPipelineReflectionVk(ShaderModuleHandle* shaderModules, size_t shaderModuleCount);
		virtual ~ShaderPipelineReflectionVk();

	private:

		void init(ShaderModuleHandle* shaderModules, size_t shaderModuleCount);
		
	};

}
