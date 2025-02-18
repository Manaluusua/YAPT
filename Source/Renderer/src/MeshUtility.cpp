#include <Renderer/MeshUtility.h>
#include <Math/MathUtility.h>
#include <vector>
#include <algorithm>
namespace YAPT
{
	namespace MeshUtility
	{
		using namespace MathUtils;
		size_t getRequiredVertexCountForUnitSphere(size_t tesselationZenith, size_t tesselationAzimuth)
		{
			tesselationZenith = std::max(tesselationZenith, size_t(3));
			tesselationAzimuth = std::max(tesselationAzimuth, size_t(3));

			return tesselationZenith * tesselationAzimuth;
		}
		size_t getRequiredIndexCountForUnitSphere(size_t tesselationZenith, size_t tesselationAzimuth, bool makeLineListInsteadOfTriangles)
		{
			tesselationZenith = std::max(tesselationZenith, size_t(3));
			tesselationAzimuth = std::max(tesselationAzimuth, size_t(3));
			return makeLineListInsteadOfTriangles ? ((1 + tesselationZenith) * tesselationAzimuth + tesselationZenith * tesselationAzimuth) * 2
				: ((tesselationZenith - 1) * 2 * tesselationAzimuth) * 3;
		}

		void generateUnitSphere(size_t tesselationZenith, size_t tesselationAzimuth, bool generateNormals, bool generateTangents, bool generateUVs, bool makeLineListInsteadOfTriangles, vec3p* positions, uint32_t* indices, vec3p* normals, vec4p* tangents, vec2p* uvs)
		{

			//tangents are generated only if normals and uvs are also generated
			generateTangents = generateNormals && generateTangents && generateUVs;


			///Initialize needed arrays
			size_t pointCount = getRequiredVertexCountForUnitSphere(tesselationZenith, tesselationAzimuth);

			//index count depends if we are drawing lines or triangles
			size_t indexCount = getRequiredIndexCountForUnitSphere(tesselationZenith, tesselationAzimuth, makeLineListInsteadOfTriangles);

			

			///generate data
			//vertex data
			const float zenithMax = glm::pi<float>();

			const float azimuthMax = 2.f * glm::pi<float>();
			size_t posIndex = 0;

			for (size_t z = 0; z < tesselationZenith; ++z) {

				float currentZenithRatio = static_cast<float>(z) / (tesselationZenith - 1);
				float currentZenith = currentZenithRatio * zenithMax;

				for (size_t a = 0; a < tesselationAzimuth; ++a) {
					size_t pointIndex = z * tesselationAzimuth + a;

					float currentAzimuthRatio = static_cast<float>(a) / (tesselationAzimuth);
					float currentAzimuth = currentAzimuthRatio * azimuthMax;

					float cosZ = cos(currentZenith);
					float sinZ = sin(currentZenith);
					float cosA = cos(currentAzimuth);
					float sinA = sin(currentAzimuth);

					positions[pointIndex] = { sinZ * cosA, cosZ, sinZ * sinA };

					if (generateUVs) {
						uvs[pointIndex].x = currentAzimuthRatio;
						uvs[pointIndex].y = currentZenithRatio;
					}

				}
			}

			if (generateNormals) {
				//since we have a unit sphere, normals == positions
				for (size_t i = 0; i < pointCount; ++i) {

					normals[i] = positions[i];
				}
			}

			if (generateTangents) {
				std::vector<vec3p> tempTan;
				tempTan.resize(pointCount);
				std::vector<vec3p> tempBitan;
				tempBitan.resize(pointCount);


				for (unsigned int z = 0; z < (tesselationZenith - 1); ++z) {
					for (unsigned int a = 0; a < tesselationAzimuth; ++a) {
						size_t pointIndex1 = z * tesselationAzimuth + a;
						size_t pointIndex2 = z * tesselationAzimuth + (a + 1) % tesselationAzimuth;
						size_t pointIndex3 = ((z + 1) % tesselationZenith) * tesselationAzimuth + a;
						size_t pointIndex4 = ((z + 1) % tesselationZenith) * tesselationAzimuth + (a + 1) % tesselationAzimuth;
						//tri1
						const vec3p& p1 = positions[pointIndex1];
						const vec3p& p2 = positions[pointIndex2];
						const vec3p& p3 = positions[pointIndex3];
						const vec3p& p4 = positions[pointIndex4];

						const vec2p& t1 = uvs[pointIndex1];
						const vec2p& t2 = uvs[pointIndex2];
						const vec2p& t3 = uvs[pointIndex3];
						const vec2p& t4 = uvs[pointIndex4];

						vec3p tangent1;
						vec3p tangent2;
						vec3p bitangent1;
						vec3p bitangent2;

						calculateTangentAndBitangentFromPositionAndUv(p1, p4, p3, t1, t4, t3, tangent1, bitangent1);
						calculateTangentAndBitangentFromPositionAndUv(p4, p1, p2, t4, t1, t2, tangent2, bitangent2);

						tempTan[pointIndex1] += tangent1;
						tempTan[pointIndex3] += tangent1;
						tempTan[pointIndex4] += tangent1;

						tempBitan[pointIndex1] += bitangent1;
						tempBitan[pointIndex3] += bitangent1;
						tempBitan[pointIndex4] += bitangent1;

						tempTan[pointIndex1] += tangent2;
						tempTan[pointIndex2] += tangent2;
						tempTan[pointIndex4] += tangent2;

						tempBitan[pointIndex1] += bitangent2;
						tempBitan[pointIndex2] += bitangent2;
						tempBitan[pointIndex4] += bitangent2;

					}
				}

				for (size_t i = 0; i < pointCount; ++i) {
					const vec3p& normal = normals[i];
					vec3p& tangent = tempTan[i];
					vec3p& bitangent = tempBitan[i];
					vec3p newTangent;
					orthogonalizeAndNormalizeTangent(tangent, normal, newTangent);
					tangents[i] = vec4p(newTangent[0], newTangent[1], newTangent[2], calculateHandedness(tangent, bitangent, normal));

				}


			}

			//indexData
			size_t index = 0;
			if (makeLineListInsteadOfTriangles) {
				//horizontal lines
				for (size_t z = 1; z < (tesselationZenith - 1); ++z) {
					for (size_t a = 0; a < tesselationAzimuth; ++a) {
						uint32_t pointIndex1 = uint32_t(z * tesselationAzimuth + a);
						uint32_t pointIndex2 = uint32_t(z * tesselationAzimuth + (a + 1) % tesselationAzimuth);
						indices[index++] = pointIndex1;
						indices[index++] = pointIndex2;
					}
				}

				//vertical lines
				for (size_t a = 0; a < tesselationAzimuth; ++a) {
					for (size_t z = 0; z < tesselationZenith; ++z) {
						uint32_t pointIndex1 = uint32_t(z * tesselationAzimuth + a);
						uint32_t pointIndex2 = uint32_t(((z + 1) % tesselationZenith) * tesselationAzimuth + a);
						indices[index++] = pointIndex1;
						indices[index++] = pointIndex2;
					}
				}

			}
			else {
				//triangles
				for (size_t z = 0; z < (tesselationZenith - 1); ++z) {
					for (size_t a = 0; a < tesselationAzimuth; ++a) {
						uint32_t pointIndex1 = uint32_t(z * tesselationAzimuth + a);
						uint32_t pointIndex2 = uint32_t(z * tesselationAzimuth + (a + 1) % tesselationAzimuth);
						uint32_t pointIndex3 = uint32_t(((z + 1)) * tesselationAzimuth + a);
						uint32_t pointIndex4 = uint32_t(((z + 1)) * tesselationAzimuth + (a + 1) % tesselationAzimuth);
						//tri1
						indices[index++] = pointIndex1;
						indices[index++] = pointIndex4;
						indices[index++] = pointIndex3;

						//tri2
						indices[index++] = pointIndex4;
						indices[index++] = pointIndex1;
						indices[index++] = pointIndex2;
					}
				}

			}
		}


	}

}