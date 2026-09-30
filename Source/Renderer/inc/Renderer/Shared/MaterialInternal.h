#pragma once

#include <Common/RCObjectPtr.h>
#include <Math/Math.h>
#include <Common/BubbleArray.h>

namespace YAPT
{
	typedef size_t MaterialIndex;
	constexpr MaterialIndex InvalidMaterialId = InvalidBubbleArrayIndex;

	static const uint32_t TEX_UNBOUND_INDEX = ~0;
	enum MaterialMask
	{
		MaterialMask_None = 0,
		MaterialMask_TwoSided = 1 << 0,
		MaterialMask_Dispersion = 1 << 1,
		MaterialMask_AlphaBlend = 1 << 2
	};

	struct MaterialParameterTexture
	{
		MaterialParameterTexture(uint32_t texIndex, float scaleX = 1, float scaleY = 1)
			:textureIndex(texIndex),
			packedScale(glm::packHalf2x16(vec2p(scaleX, scaleY)))
		{

		}

		void setScale(float scaleX, float scaleY)
		{
			packedScale = glm::packHalf2x16(vec2p(scaleX, scaleY));
		}

		uint32_t textureIndex;
		uint32_t packedScale;
	};

	struct MaterialParameters
	{
		MaterialParameters()
			:albedo(1,1,1),
			transparency(0),
			specular(1,1,1),
			metalness(1),
			absorption(0,0,0),
			dielectricIOR(1.5),
			emissive(0,0,0),
			roughness(0.1),
			anisotropy(0),
			anisotropyRotation(0),
			materialMask(0),
			specularAmount(1.f),
			clearCoatAmount(0.f),
			clearCoatIOR(1.5f),
			clearCoatRoughness(0.1f),
			sheenTint(1.f, 1.f, 1.f),
			sheenRoughness(0.5f),

			sheenAmount(0.f),
			thinFilmThicknessNM(0.f),
			emissionFocus(0.f),
			alphaCutoff(0.f),

			cauchysCoeffs(1.5046f, 0.00420f),

			albedoTex(TEX_UNBOUND_INDEX),
			normalTex(TEX_UNBOUND_INDEX),
			ormTex(TEX_UNBOUND_INDEX),
			emissiveTex(TEX_UNBOUND_INDEX)
		{}

		MaterialParameters(vec3p albedo, float transparency,
		vec3p specular, float metalness,
		vec3p absorption, float dielectricIOR,
		vec3p emissive, float roughness,
		float anisotropy, float anisotropyRotation)
			:albedo(albedo),
			transparency(transparency),
			specular(specular),
			metalness(metalness),
			absorption(absorption),
			dielectricIOR(dielectricIOR),
			emissive(emissive),
			roughness(roughness),
			anisotropy(anisotropy),
			anisotropyRotation(anisotropyRotation),
			materialMask(0),
			specularAmount(1.f),
			clearCoatAmount(0.f),
			clearCoatIOR(1.5f),
			clearCoatRoughness(0.1f),
			sheenTint(1.f, 1.f, 1.f),
			sheenRoughness(0.5f),

			sheenAmount(0.f),
			thinFilmThicknessNM(0.f),
			emissionFocus(0.f),
			alphaCutoff(0.f),

			cauchysCoeffs(1.5046f, 0.00420f),

			albedoTex(TEX_UNBOUND_INDEX),
			normalTex(TEX_UNBOUND_INDEX),
			ormTex(TEX_UNBOUND_INDEX),
			emissiveTex(TEX_UNBOUND_INDEX)
		{}
		
		vec3p albedo;
		float transparency;
		vec3p specular;
		float metalness;
		vec3p absorption;
		float dielectricIOR;
		vec3p emissive;
		float roughness;

		float anisotropy;
		float anisotropyRotation;

		uint32_t materialMask;

		float specularAmount;
		float clearCoatAmount;
		float clearCoatIOR;
		float clearCoatRoughness;

		vec3p sheenTint;
		float sheenRoughness;

		float sheenAmount;
		float thinFilmThicknessNM;
		float emissionFocus;
		float alphaCutoff; //0 = opaque, otherwise hits where the albedo texture alpha is below this are ignored

		vec2p cauchysCoeffs;

		MaterialParameterTexture albedoTex;
		MaterialParameterTexture normalTex;
		MaterialParameterTexture ormTex;
		MaterialParameterTexture emissiveTex;

	};


	class MaterialInternal
	{
	public:
		MaterialInternal(MaterialIndex id);
		~MaterialInternal();

		void setMaterialParams(const MaterialParameters& params);
		const MaterialParameters& getMaterialParams() const;
		MaterialIndex getID() const { return m_id; }

	private:
		void validateMaterial();

		MaterialParameters m_materialParams;
		MaterialIndex m_id;
	};
}

