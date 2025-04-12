#pragma once

#include <Common/RCObjectPtr.h>
#include <Math/Math.h>
namespace YAPT
{
	static const uint32_t TEX_UNBOUND_INDEX = ~0;
	enum MaterialMask
	{
		MaterialMask_None = 0,
		MaterialMask_TwoSided = 1 << 0,
		MaterialMask_Dispersion = 1 << 1
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
			thinFilmThickness(0.f),

			cauchysCoeffs(1.5046f, 0.00420f),

			albedoTexIndex(TEX_UNBOUND_INDEX),
			normalTexIndex(TEX_UNBOUND_INDEX),
			ormTexIndex(TEX_UNBOUND_INDEX),
			emissiveTexIndex(TEX_UNBOUND_INDEX)
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
			thinFilmThickness(0.f),

			cauchysCoeffs(1.5046f, 0.00420f),

			albedoTexIndex(TEX_UNBOUND_INDEX),
			normalTexIndex(TEX_UNBOUND_INDEX),
			ormTexIndex(TEX_UNBOUND_INDEX),
			emissiveTexIndex(TEX_UNBOUND_INDEX)
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
		float thinFilmThickness;

		vec2p cauchysCoeffs;

		uint32_t albedoTexIndex;
		uint32_t normalTexIndex;
		uint32_t ormTexIndex;
		uint32_t emissiveTexIndex;
	};


	class MaterialInternal
	{
	public:
		MaterialInternal(size_t id);
		~MaterialInternal();

		void setMaterialParams(const MaterialParameters& params);
		const MaterialParameters& getMaterialParams() const;

	private:
		MaterialParameters m_materialParams;
		size_t m_id;
	};
}

