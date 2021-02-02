#pragma once

#include <Renderer/Shared/CRendererConfiguration.h>
#include <Renderer/Shared/CRenderer.h>

#define INIT_RENDERVARSLIST_EMPTY(NAME, TYPE) config->registerVariable(NAME, TYPE);
#define INIT_RENDERVARSLIST_DEFAULTS(NAME, TYPE, DEFAULT) config->registerVariable(NAME, TYPE)->set(DEFAULT);
#define INIT_RENDERVARSLIST_LIMITS(NAME, TYPE, DEFAULT, MIN, MAX) {auto var = static_cast<RendererVariableInternal*>(config->registerVariable(NAME, TYPE)); var->set(DEFAULT); var->setLimits(MIN, MAX);}

namespace YAPT
{
	inline void initRVars(CRendererConfiguration* config)
	{
		//general
		INIT_RENDERVARSLIST_EMPTY(RVARNAME_SKYBOX, RendererVariableType::TEXTURE);
		INIT_RENDERVARSLIST_DEFAULTS(RVARNAME_RENDER_RESOLUTION, RendererVariableType::IVEC2, glm::ivec2(1920, 1080));

		//Tonemap
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_TOE, RendererVariableType::FLOAT, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_MID, RendererVariableType::FLOAT, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_SHOULDER, RendererVariableType::FLOAT, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_USE_AUTOEXPOSURE, RendererVariableType::UINT, 1u, 0u, 1u);
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_EXPOSURE_COMPENSATION, RendererVariableType::FLOAT, 0.f, -10000.f, 10000.f);
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_EYE_ADAPT_SPEED, RendererVariableType::FLOAT, 1.f, 0.f, 1000.f);
		INIT_RENDERVARSLIST_LIMITS(RVARNAME_TONEMAP_MANUALEXPOSURE, RendererVariableType::FLOAT, 1.f, 0.f, 1000.f);

		//Test
		/*INIT_RENDERVARSLIST_LIMITS("Test.float", RendererVariableType::FLOAT, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_LIMITS("Test.int", RendererVariableType::INT, 1, 0, 100);
		INIT_RENDERVARSLIST_LIMITS("Test.uint", RendererVariableType::UINT, 1u, 0u, 100u);

		INIT_RENDERVARSLIST_LIMITS("Test.vec2", RendererVariableType::VEC2, glm::vec2(2), glm::vec2(0), glm::vec2(3));
		INIT_RENDERVARSLIST_LIMITS("Test.vec3", RendererVariableType::VEC3, glm::vec3(1), glm::vec3(0), glm::vec3(2));
		INIT_RENDERVARSLIST_LIMITS("Test.vec4", RendererVariableType::VEC4, glm::vec4(1), glm::vec4(0), glm::vec4(2));

		
		INIT_RENDERVARSLIST_LIMITS("Test.ivec2", RendererVariableType::IVEC2, glm::ivec2(2), glm::ivec2(0), glm::ivec2(3));
		INIT_RENDERVARSLIST_LIMITS("Test.ivec3", RendererVariableType::IVEC3, glm::ivec3(1), glm::ivec3(0), glm::ivec3(2));
		INIT_RENDERVARSLIST_LIMITS("Test.ivec4", RendererVariableType::IVEC4, glm::ivec4(1), glm::ivec4(0), glm::ivec4(2));
		*/




		config->commitChanges();
	}

}