#pragma once

#include <Gfx/GfxTypes.h>
#include <Gfx/ShaderPipelineReflection.h>
#include <unordered_map>
namespace YAPT
{
	

	class ShaderLoader
	{
	public:
		struct ShaderModuleInfo
		{
			ShaderModuleType moduletype;
			ShaderModuleHandle handle;
			const char* entryPoint;
		};

		struct ShaderPipelineInfo
		{
			std::vector<ShaderModuleInfo> shaderModules;
			ShaderPipelineReflection* reflection;
		};


		ShaderLoader(GfxApiHandle gfx);
		~ShaderLoader();

		const ShaderPipelineInfo* getShaderPipeline(const std::string& name);

	private:

		struct ShaderStageDefine
		{
			std::string name;
			std::string value;
		};

		struct ShaderStageDefinition
		{
			ShaderModuleType moduletype;
			std::string filePath;
			std::string entryPoint;
			std::vector<ShaderStageDefine> definitions;
		};

		struct ShaderPipelineInternal
		{
			std::vector<ShaderStageDefinition> stages;
			ShaderPipelineInfo info;
		};

		void loadJson();
		void loadDefaultShaders();


		GfxApiHandle m_gfx;
		std::unordered_map<std::string, ShaderPipelineInternal> m_loadedShaderPipelines;
	};

	

}