#pragma once
#include <PyBindingsCommon.h>
#include <Common/RCObject.h>
#include <Renderer/Material.h>
#include <memory>
namespace YAPT
{
	class Renderer;
	class PyMaterial : public RCObject
	{
	public:
		DECLARE_BINDING_CLASS(PyMaterial);

		PyMaterial(Renderer* rend, const char* name);
		~PyMaterial();

		Material* getMaterial() { return m_material; }

		const char* getName() const;
		void setFromMaterialPreset(MaterialPreset preset);

		void setTransparency(float transparency);
		float getTransparency() const;

		void setMetalness(float metalness);;
		float getMetalness() const;

		void setAlbedo(const vec3p& v);
		const vec3p& getAlbedo() const;

		void setSpecularTint(const vec3p& v);
		const vec3p& getSpecularTint() const;

		void setAbsorption(const vec3p& v);
		const vec3p& getAbsorption() const;

		void setEmission(const vec3p& v);
		const vec3p& getEmission() const;

		void setDielectricIOR(float ior);
		float getDielectricIOR() const;

		void setRoughness(float roughness);
		float getRoughness() const;

		void setAnisotropy(float anisotropy);
		float getAnisotropy() const;

		void setAnisotropyRotation(float rot);
		float getAnisotropyRotation() const;

		void setTwoSided(bool val);
		bool getTwoSided() const;

		void setSpecularAmount(float val);
		float getSpecularAmount() const;

		void setClearCoatAmount(float val);
		float getClearCoatAmount() const;

		void setClearCoatRoughness(float val);
		float getClearCoatRoughness() const;

		void setClearCoatIOR(float val);
		float getClearCoatIOR() const;

		void setSheenRoughness(float val);
		float getSheenRoughness() const;

		void setSheenTint(const vec3p& v);
		const vec3p& getSheenTint() const;

		void setSheenAmount(float val);
		float getSheenAmount() const;

		void setThinFilmThickness(float val);
		float getThinFilmThickness() const;

		void setCauchysCoefficients(const vec2p& val);
		const vec2p& getCauchysCoefficients() const;

		void setEnableDispersion(bool val);
		bool getEnableDispersion() const;

	private:
		std::string m_name;
		RCObjectPtr<Material> m_material;
	};
}