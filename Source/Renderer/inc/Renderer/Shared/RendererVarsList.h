#pragma once

#include <Renderer/Shared/CRendererConfiguration.h>
#include <Renderer/Shared/CRenderer.h>

#define INIT_LIST(...) {__VA_ARGS__}

#define INIT_RENDERVARSLIST_TEXTURE(NAME) config->registerVariable(NAME, new CRendererVariableResource<Texture, RenderVariableTextureInternal, RendererVariableType::TEXTURE>(*config));
#define INIT_RENDERVARSLIST_BUFFER(NAME) config->registerVariable(NAME,  new CRendererVariableResource<Buffer, RenderVariableBufferInternal, RendererVariableType::BUFFER>(*config));

#define INIT_RENDERVARSLIST_INT(NAME, DEFAULT, MIN, MAX) config->registerVariable(NAME, constructNumericRenderVar<RenderVariableIntInternal, RendererVariableType::INT>(*config, DEFAULT, MIN, MAX));
#define INIT_RENDERVARSLIST_FLOAT(NAME, DEFAULT, MIN, MAX) config->registerVariable(NAME, constructNumericRenderVar<RenderVariableFloatInternal, RendererVariableType::FLOAT>(*config, DEFAULT, MIN, MAX));

#define INIT_RENDERVARSLIST_OPTIONS(NAME, SELECTED, OPTIONS) {const char* optionsList[] = INIT_LIST OPTIONS; config->registerVariable(NAME, new CRendererVariableOptions<sizeof(optionsList)/sizeof(char*)>(*config, SELECTED, optionsList));}

namespace YAPT
{
	inline void initRVars(CRendererConfiguration* config)
	{
		//general
		INIT_RENDERVARSLIST_TEXTURE(RVARNAME_SKYBOX);
		INIT_RENDERVARSLIST_INT(RVARNAME_RENDER_RESOLUTION, ivec2p(1920, 1080), ivec2p(2, 2), ivec2p(99999999, 99999999));
		INIT_RENDERVARSLIST_OPTIONS(RVARNAME_ACTIVE_RENDERPIPELINE, 0, ("SimplePT", "BDPT", "DBG_Normals", "DBG_Tangents", "DBG_UV") )

		//Tonemap
		INIT_RENDERVARSLIST_FLOAT(RVARNAME_TONEMAP_TOE, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_FLOAT(RVARNAME_TONEMAP_MID, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_FLOAT(RVARNAME_TONEMAP_SHOULDER, 1.f, 0.f, 100.f);
		INIT_RENDERVARSLIST_OPTIONS(RVARNAME_TONEMAP_USE_AUTOEXPOSURE, 0 , ("OFF", "ON"));
		INIT_RENDERVARSLIST_FLOAT(RVARNAME_TONEMAP_EXPOSURE_COMPENSATION, 0.f, -10000.f, 10000.f);
		INIT_RENDERVARSLIST_FLOAT(RVARNAME_TONEMAP_EYE_ADAPT_SPEED, 1.f, 0.f, 1000.f);
		INIT_RENDERVARSLIST_FLOAT(RVARNAME_TONEMAP_MANUALEXPOSURE, 1.f, 0.f, 1000.f);

		config->commitChanges();
	}

}