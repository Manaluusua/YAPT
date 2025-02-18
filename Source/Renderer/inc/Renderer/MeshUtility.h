#ifndef YAPT_MESHUTILITY_H
#define YAPT_MESHUTILITY_H

#include "CommonDefines.h"
#include <Math/Math.h>

namespace YAPT
{
	namespace MeshUtility
	{
		size_t getRequiredVertexCountForUnitSphere(size_t tesselationZenith, size_t tesselationAzimuth);
		size_t getRequiredIndexCountForUnitSphere(size_t tesselationZenith, size_t tesselationAzimuth, bool makeLineListInsteadOfTriangles);
		void generateUnitSphere(size_t tesselationZenith, size_t tesselationAzimuth, bool generateNormals, bool generateTangents, bool generateUVs, bool makeLineListInsteadOfTriangles,
			vec3p* positions, uint32_t* indices, vec3p* normals, vec4p* tangents, vec2p* uvs);


	}
}

#endif