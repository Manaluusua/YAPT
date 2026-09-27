#pragma once

#include <Renderer/Material.h>
#include <Renderer/Shared/MaterialManager.h>
#include <Renderer/Shared/MaterialInternal.h>
#include <Gfx/GfxBasicTypes.h>

namespace YAPT
{
	class MaterialProxy : public Material
	{
		friend class MaterialManager;
	public:

		enum MaterialState
		{
			MATERIALSTATE_NOCHANGES = 0,
			MATERIALSTATE_CREATED = YAPTBIT(1),
			MATERIALSTATE_MODIFIED = YAPTBIT(2),
			MATERIALSTATE_DESTROYED = YAPTBIT(3)
		};

		MaterialProxy(MaterialManager* mngr);
		virtual ~MaterialProxy();

		MaterialInternal* getMaterialInternal() { return m_mngr->getMaterialInternal(_id); }

		virtual void setFromMaterialPreset(MaterialPreset preset) final;

		virtual void setTransparency(float transparency) final;
		virtual float getTransparency() const final;

		virtual void setMetalness(float metalness) final;;
		virtual float getMetalness() const final;

		virtual void setAlbedo(const vec3p& v) final;
		virtual const vec3p& getAlbedo() const final;

		virtual void setSpecularTint(const vec3p& v) final;
		virtual const vec3p& getSpecularTint() const final;

		virtual void setAbsorption(const vec3p& v) final;
		virtual const vec3p& getAbsorption() const final;

		virtual void setEmission(const vec3p& v) final;
		virtual const vec3p& getEmission() const final;

		virtual void setEmissionFocus(float focus) final;
		virtual float getEmissionFocus() const final;

		virtual void setDielectricIOR(float ior) final;
		virtual float getDielectricIOR() const final;

		virtual void setRoughness(float roughness) final;
		virtual float getRoughness() const final;

		virtual void setAnisotropy(float anisotropy) final;
		virtual float getAnisotropy() const final;

		virtual void setAnisotropyRotation(float rot) final;
		virtual float getAnisotropyRotation() const final;

		virtual void setTwoSided(bool val) final;
		virtual bool isTwoSided() const final;

		virtual void setSpecularAmount(float val) final;
		virtual float getSpecularAmount() const final;

		virtual void setClearCoatAmount(float val) final;
		virtual float getClearCoatAmount() const final;

		virtual void setClearCoatRoughness(float val) final;
		virtual float getClearCoatRoughness() const final;

		virtual void setClearCoatIOR(float val) final;
		virtual float getClearCoatIOR() const final;

		virtual void setSheenRoughness(float val) final;
		virtual float getSheenRoughness() const final;

		virtual void setSheenTint(const vec3p& v) final;
		virtual const vec3p& getSheenTint() const final;

		virtual void setSheenAmount(float val) final;
		virtual float getSheenAmount() const final;

		virtual void setThinFilmThicknessNM(float val) final;
		virtual float getThinFilmThicknessNM() const final;

		virtual void setAlphaCutoff(float cutoff) final;
		virtual float getAlphaCutoff() const final;

		virtual void setCauchysCoefficients(const vec2p& val) final;
		virtual const vec2p& getCauchysCoefficients() const final;

		virtual void setEnableDispersion(bool val) final;
		virtual bool getEnableDispersion() const final;

		virtual void setAlbedoTexture(const TextureParameter& tex) final;
		virtual void setNormalTexture(const TextureParameter& tex) final;
		virtual void setORMTexture(const TextureParameter& tex) final;
		virtual void setEmissiveTexture(const TextureParameter& tex) final;

	protected:
		virtual void allReferencesReleased() final;

	private:
		void assignMaterialTextureParam(const TextureParameter& source, MaterialParameterTexture& dest);
		uint32_t getTextureViewIndex(Texture* tex);

		void setDirty();

		MaterialParameters m_materialParams;
		MaterialManager* m_mngr;

		//Handled by MaterialManager
		size_t _materialState;
		MaterialIndex _id;
	};
}
