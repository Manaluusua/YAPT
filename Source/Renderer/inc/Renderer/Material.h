#ifndef YAPT_MATERIAL_H
#define YAPT_MATERIAL_H

#include <Common/RCObject.h>
#include <Common/RCObjectPtr.h>
#include <Math/Math.h>


namespace YAPT
{
	class Texture;
	enum class MaterialPreset
	{
		BLANK,
		METAL_GOLD,
		METAL_SILVER,
		METAL_COPPER,
		METAL_BRASS,
		METAL_ALUMINIUM,
		GLASS,
		PLASTIC,
	};

	class Material : public RCObject
	{
	public:
		virtual void setFromMaterialPreset(MaterialPreset preset) = 0;

		virtual void setTransparency(float transparency) = 0;
		virtual float getTransparency() const = 0;

		virtual void setMetalness(float metalness) = 0;;
		virtual float getMetalness() const = 0;

		virtual void setAlbedo(const vec3p& v) = 0;
		virtual const vec3p& getAlbedo() const = 0;

		virtual void setSpecularTint(const vec3p& v) = 0;
		virtual const vec3p& getSpecularTint() const = 0;

		virtual void setAbsorption(const vec3p& v) = 0;
		virtual const vec3p& getAbsorption() const = 0;

		virtual void setEmission(const vec3p& v) = 0;
		virtual const vec3p& getEmission() const = 0;

		virtual void setDielectricIOR(float ior) = 0;
		virtual float getDielectricIOR() const = 0;

		virtual void setRoughness(float roughness) = 0;
		virtual float getRoughness() const = 0;

		virtual void setAnisotropy(float anisotropy) = 0;
		virtual float getAnisotropy() const = 0;

		virtual void setAnisotropyRotation(float rot) = 0;
		virtual float getAnisotropyRotation() const = 0;

		virtual void setTwoSided(bool val) = 0;
		virtual bool isTwoSided() const = 0;

		virtual void setSpecularAmount(float val) = 0;
		virtual float getSpecularAmount() const = 0;

		virtual void setClearCoatAmount(float val) = 0;
		virtual float getClearCoatAmount() const = 0;

		virtual void setClearCoatRoughness(float val) = 0;
		virtual float getClearCoatRoughness() const = 0;

		virtual void setClearCoatIOR(float val) = 0;
		virtual float getClearCoatIOR() const = 0;

		virtual void setSheenAmount(float val) = 0;
		virtual float getSheenAmount() const = 0;

		virtual void setSheenRoughness(float val) = 0;
		virtual float getSheenRoughness() const = 0;

		virtual void setSheenTint(const vec3p& v) = 0;
		virtual const vec3p& getSheenTint() const = 0;

		virtual void setThinFilmThickness(float val) = 0;
		virtual float getThinFilmThickness() const = 0;

		virtual void setAlbedoTexture(RCObjectPtr<Texture>& tex) = 0;
		virtual void setNormalTexture(RCObjectPtr<Texture>& tex) = 0;
		virtual void setORMTexture(RCObjectPtr<Texture>& tex) = 0; //Occlusion, Roughness, Metalness
		virtual void setEmissiveTexture(RCObjectPtr<Texture>& tex) = 0;
	};
}

#endif