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
		uvec4p targetTextureDimensions;
		vec4p clearValue;
	};

	struct MergeNewSamplesParams
	{
		uvec4p targetTextureOffsetScaleBias;
		uvec2p sourceTextureDimensions;
	};
} 