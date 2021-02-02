#pragma once

#include <Math/Math.h>



namespace YAPT
{
	struct Tex2TexAlphaFromUbo
	{
		float alpha;
	};

	struct ClearAccumulatedSamplesParams
	{
		glm::uvec4 targetTextureDimensions;
		glm::vec4 clearValue;
	};

	struct MergeNewSamplesParams
	{
		glm::uvec4 targetTextureOffsetScaleBias;
		glm::uvec2 sourceTextureDimensions;
	};
} 