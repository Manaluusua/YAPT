#include <Renderer/Shared/Utility/ShaderLoader.h>
#include <Renderer/Shared/GfxApi.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <fstream>
#include <Common/CommonUtilities.h>
#include <Common/Logger.h>

const char* jsonFilesToLoad[] = {
	"shaders/postProcessPipelineDefinitions.json",
	"shaders/rayTracePipelineDefinitions.json",
	"shaders/toolsPipelineDefinitions.json"
};

namespace YAPT
{
	using namespace nlohmann;

	bool stringToShaderModuleType(const std::string& str, ShaderModuleType& shdModuleOut)
	{
		const static std::unordered_map<std::string, ShaderModuleType> stringToShaderModuleType
		{
			{"vertex", ShaderModuleType::VERTEX_MODULE},
			{"hull", ShaderModuleType::HULL_MODULE},
			{"domain", ShaderModuleType::DOMAIN_MODULE},
			{"geometry", ShaderModuleType::GEOMETRY_MODULE},
			{"fragment", ShaderModuleType::FRAGMENT_MODULE},
			{"compute",ShaderModuleType::COMPUTE_MODULE},
			{"library",ShaderModuleType::LIBRARY_MODULE},


		};

		auto iter = stringToShaderModuleType.find(str);
		if (iter == stringToShaderModuleType.end())
		{
			return false;
		}

		shdModuleOut = iter->second;

		return true;
	}


	ShaderLoader::ShaderLoader(GfxApiHandle gfx)
		:m_gfx(gfx)
	{
		loadDefaultShaders();
	}

	ShaderLoader::~ShaderLoader()
	{
		for (auto pipelineIter = m_loadedShaderPipelines.begin(); pipelineIter != m_loadedShaderPipelines.end(); ++pipelineIter)
		{
			ShaderLoader::ShaderPipelineInfo& shaderPipelineInfo = pipelineIter->second.info;

			Gfx::destroyShaderPipelineReflection(shaderPipelineInfo.reflection);

			for (size_t stageIndex = 0; stageIndex < shaderPipelineInfo.shaderModules.size(); ++stageIndex)
			{
				Gfx::destroyShaderModule(m_gfx, shaderPipelineInfo.shaderModules[stageIndex].handle);
			}

			
		}
	}

	const ShaderLoader::ShaderPipelineInfo* ShaderLoader::getShaderPipeline(const std::string& name)
	{

		auto iter = m_loadedShaderPipelines.find(name);
		if (iter == m_loadedShaderPipelines.end())
		{
			return nullptr;
		}

		return &iter->second.info;
	}


	void ShaderLoader::loadDefaultShaders()
	{
		loadJson();

		std::vector<ShaderModuleDefine> definesTemp;
		definesTemp.reserve(64);

		std::vector< ShaderModuleHandle> shaderModHandles;
		shaderModHandles.reserve(64);

		static const std::string shaderFolderRootPath = "shaders/";

		std::string constructedPathTemp;

		for (auto pipelineIter = m_loadedShaderPipelines.begin(); pipelineIter != m_loadedShaderPipelines.end(); ++pipelineIter)
		{
			ShaderPipelineInternal& shaderPipelineDef = pipelineIter->second;
			
			shaderPipelineDef.info.shaderModules.resize(shaderPipelineDef.stages.size());

			shaderModHandles.clear();

			for (size_t stageIndex = 0; stageIndex < shaderPipelineDef.stages.size(); ++stageIndex)
			{
				ShaderStageDefinition& stageDef = shaderPipelineDef.stages[stageIndex];

				definesTemp.clear();
				for (size_t i = 0; i < stageDef.definitions.size(); ++i)
				{
					definesTemp.push_back({ stageDef.definitions[i].name.c_str(), stageDef.definitions[i].value.c_str() });
				}


				std::string pathToShaderFile = shaderFolderRootPath + stageDef.filePath;
				ShaderModuleHandle shdMod = Gfx::createShaderModuleFromFile(m_gfx, pathToShaderFile.c_str(), stageDef.moduletype, stageDef.entryPoint.c_str(), definesTemp.data(), definesTemp.size());
				assert(shdMod != YAPT_NULL_HANDLE);
				shaderPipelineDef.info.shaderModules[stageIndex].handle = shdMod;
				shaderPipelineDef.info.shaderModules[stageIndex].moduletype = stageDef.moduletype;
				shaderPipelineDef.info.shaderModules[stageIndex].entryPoint = stageDef.entryPoint.c_str();

				shaderModHandles.push_back(shdMod);
			}

			shaderPipelineDef.info.reflection = Gfx::createShaderPipelineReflection(shaderModHandles.data(), shaderModHandles.size());
		}
	 
	}

	void ShaderLoader::loadJson()
	{
		size_t jsonFileCount = countOf(jsonFilesToLoad);

		for (size_t i = 0; i < jsonFileCount; ++i)
		{
			std::ifstream ifs(jsonFilesToLoad[i]);

			json shaderPipelineDefinitions = json::parse(ifs);
			const json& shaderPipelineObjects = shaderPipelineDefinitions["shaderPipelines"];

			for (auto pipelineIter = shaderPipelineObjects.begin(); pipelineIter != shaderPipelineObjects.end(); ++pipelineIter)
			{
				const std::string& name = pipelineIter.key();
				ShaderPipelineInternal& pipelineDef = m_loadedShaderPipelines[name];

				assert(pipelineDef.stages.size() == 0);
				if (pipelineDef.stages.size() != 0)
				{
					YAPT_LOG_ERROR("Already loaded shaderpipeline %s, skipping...", name.c_str());
					continue;
				}

				const json& stages = pipelineIter->at("stages");
				pipelineDef.stages.reserve(stages.size());
				for (auto stagesIter = stages.begin(); stagesIter != stages.end(); ++stagesIter)
				{
					const std::string& type = stagesIter->at("type");
					ShaderModuleType moduletype;
					bool success = stringToShaderModuleType(type, moduletype);
					if (!success)
					{
						YAPT_LOG_ERROR("Error in loading shaderpipeline definition %s: Pipeline stage %s not recognized", name.c_str(), type.c_str());
						continue;
					}

					pipelineDef.stages.resize(pipelineDef.stages.size() + 1);
					ShaderStageDefinition& stageDef = pipelineDef.stages.back();
					stageDef.moduletype = moduletype;
					stageDef.filePath = stagesIter->at("sourceFile");
					stageDef.entryPoint = stagesIter->contains("entryPoint") ? stagesIter->at("entryPoint") : "main";


					if (stagesIter->contains("defines"))
					{
						const json& defines = stagesIter->at("defines");
						stageDef.definitions.reserve(defines.size());
						for (auto defineIter = defines.begin(); defineIter != defines.end(); ++defineIter)
						{
							if (defineIter->is_array())
							{
								stageDef.definitions.push_back({ defineIter.value()[0], defineIter.value()[1] });
							}
							else
							{
								stageDef.definitions.push_back({ defineIter.value(), "1" });
							}

						}
					}

				}

			}
		}

		
		
	}


}